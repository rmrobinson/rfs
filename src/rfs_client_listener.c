#include <assert.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdio.h> // @todo remove this

#ifdef __APPLE__
#include <sys/ucred.h>
#endif

#include <uv.h>

#include "rfs_client.h"
#include "rfs_util.h"

#ifndef UNIX_PATH_MAX
 #ifdef __APPLE__
#define UNIX_PATH_MAX 104
 #endif
#endif

/// @brief A structure representing one incoming API connection.
/// This corresponds on a 1:1 basis with an accept()ed pipe.
typedef struct rfs__client_conn {
  LIST_ENTRY(rfs__client_conn) conns; ///< The list of connections.

  uv_pipe_t pipe; ///< The accept()ed pipe this connection is using.

  char* data; ///< The buffer being used for data.
  size_t datalen; ///< The size of data
  size_t dataoff; ///< The current offset of used bytes in data.

  time_t conn_time; ///< The time this connection was accepted.
} rfs__client_conn_t;

/// @brief A structure representing one API endpoint.
/// This corresponds on a 1:1 basis with a listening pipe.
typedef struct rfs__client_listener {
  char* path; ///< The path that the pipe is listening on
  uv_pipe_t pipe; ///< The pipe listening for connections.

  /// @brief The beginning of the list of clients.
  LIST_HEAD(rfs__client_conn_head, rfs__client_conn) conns_head;
} rfs__client_listener_t;

/// @brief The listener currently active.
static rfs__client_listener_t* _listener;

/// @brief Free an allocated connection.
/// If this is being called from a uv_close_cb then we will not close
/// the pipe in this function (it has already been closed), otherwise
/// the pipe will be closed.
/// @param [in] conn The connection to close.
static void rfs__client_conn_free(rfs__client_conn_t* conn) {
  if(conn == NULL)
    return;

  if(uv_is_closing((uv_handle_t*) &(conn->pipe)) == 0) {
    uv_close((uv_handle_t*) &(conn->pipe), NULL);
  }

  LIST_REMOVE(conn, conns);

  if(conn->data != NULL)
    free(conn->data);

  free(conn);
}

/// @brief Free an allocated listener.
/// This will close and free all of the connected conns as well.
/// This will not clean up the event loop.
/// @param [in] data The object to free.
static void rfs__client_listener_free(rfs__client_listener_t* listener) {
  if(listener == NULL)
    return;

  uv_close((uv_handle_t*) &(listener->pipe), NULL);

  while(!LIST_EMPTY(&(listener->conns_head))) {
    rfs__client_conn_free(LIST_FIRST(&(listener->conns_head)));
  }

  free(listener->path);
  free(listener);
}

/// @brief Shut down the worker thread.
/// This will close the listener plus all connected clients.
/// Any clients, including the one making the request, will not receive
/// further responses, so they will need to interpret a 'sock closed'
/// as a sign the thread shut down.
/// @param [in] loop The event loop which the worker thread is running.
static void rfs__client_shutdown(uv_loop_t* loop) {
  assert(loop != NULL);
  assert(loop->data != NULL);

  rfs__client_listener_t* listener = loop->data;

  fprintf(stdout, "Shuting down tid %ld, listener at %s closing\n", rfs__gettid(), listener->path);

  uv_shutdown_t sreq;
  uv_shutdown(&sreq, (uv_stream_t*) &(listener->pipe), NULL);

  uv_fs_t ulreq; // this doesn't need any args for unlink to succeed
  uv_fs_unlink(loop, &ulreq, listener->path, NULL);

  rfs__client_listener_free(listener);
  _listener = NULL;

  uv_stop(loop);
  free(loop);
}

/// @brief Allocate a buffer to be used for reading data.
/// If the connection doesn't have a buffer, it will allocate a buffer and
/// return the entire new buffer for use.
/// If the connection has a buffer, it will return a pointer to the first
/// unused byte of the connection's existing buffer.
/// @param [in] hdl The handle that needs the buffer.
/// @param [in] suggested_size The requested buffer size; less may be provided.
/// @param [in] buf The buffer storing the memory reference.
static void rfs__client_alloc_buf(uv_handle_t* hdl,
                                  size_t suggested_size,
                                  uv_buf_t* buf) {
  assert(hdl != NULL);
  assert(hdl->data != NULL);
  assert(buf != NULL);

  rfs__client_conn_t* conn = hdl->data;

  if(conn->data == NULL) {
    conn->data = malloc(suggested_size);

    if(conn->data == NULL) {
      // log error
      rfs__client_conn_free(conn);

      buf->base = NULL;
      buf->len = 0;
      return;
    }

    conn->datalen = suggested_size;
    conn->dataoff = 0;
  }

  buf->base = conn->data + conn->dataoff;
  buf->len = conn->datalen - conn->dataoff;
}

/// @brief Invoke the requested function within the worker.
/// This takes the function request generated by any of the API threads and
/// executes it in a single-threaded, async manner within the worker thread.
/// Some functions may modify the namespace visible to other clients (bind),
/// while other functions may result in new server connections being made
/// (i.e. mount), while others may cause network requests to be made (read,
/// write, stat, ...).
/// @note This will set the ret field in the function request when execution
/// of the function is complete; callers should check func->ret for success
/// or failure details.
/// @todo Modify connections to hold references to their outstanding requests.
/// @param [in] loop The event loop which the request came in from.
/// @param [in] func The function data to use while executing the request.
static void rfs__client_on_invoke(uv_loop_t* loop, rfs__client_func_t* func) {
  assert(loop != NULL);
  assert(func != NULL);

  switch(func->type) {
    case RFS__CLIENT_SHUTDOWN:
      rfs__client_shutdown(loop);
      break;

    case RFS__CLIENT_FUNC_BIND:
      fprintf(stdout, "bind() called with name '%s', old '%s', flags %d\n",
                      func->args.bind.name, func->args.bind.old,
                      func->args.bind.flags);

      func->ret = 0;
      break;

    default:
      fprintf(stdout, "%d called\n", func->type);
      func->ret = 0;
      break;
  }
}

/// @brief Read available data from one of the connection pipes.
/// This will parse out the API protocol (i.e. the sending of uintptr values
/// directly over the pipe) and upon receiving a pointer to a function request
/// invoke it.
/// In case some data is only partially read, the connection object keeps
/// unused bytes at the beginning of its buffer; so don't start reading from
/// the argument buf's start, start from the connection's data start instead.
/// @param [in] sconn The connection which read the data.
/// @param [in] nread The number of bytes read, -1 on error. 0 is normal, and
/// unlike a POSIX socket doesn't imply that the connection should be closed.
/// @param [in] buf The buffer with the read data.
static void rfs__client_on_read(uv_stream_t* sconn,
                                ssize_t nread,
                                const uv_buf_t* buf) {
  assert(sconn != NULL);
  assert(sconn->data != NULL);
  assert(buf != NULL);

  rfs__client_conn_t* conn = sconn->data;

  if(nread < 0) {
    // log error

    rfs__client_conn_free(conn);
    return;
  }

  // the start of the conn's buffer, plus the saved offset, should be there
  // the read buffer starts.
  assert((conn->data + conn->dataoff) == buf->base);

  size_t to_process = conn->dataoff + (size_t) nread;
  assert(to_process <= conn->datalen);

  size_t processed = 0;
  for(; processed < to_process; processed += sizeof(uintptr_t) ) {
    uintptr_t func;
    memcpy(&func, &(conn->data[processed]), sizeof(uintptr_t));

    rfs__client_on_invoke(sconn->loop, (rfs__client_func_t*) func);

    uv_buf_t resp[] = { { .base = (char*) &func, .len = sizeof(func) } };

    uv_write_t req;
    uv_write(&req, sconn, resp, 1, NULL);
  }

  conn->dataoff = to_process - processed;
  memmove(conn->data, conn->data + processed, conn->dataoff);
}

/// @brief Handle an incoming connection on the listening pipe.
/// This will create a new connection object and store it in the listener.
/// @param [in] slistener The listener which has available clients.
/// @param [in] status The status of the listener.
static void rfs__client_on_connect(uv_stream_t* slistener, int status) {
  assert(slistener != NULL);
  assert(slistener->loop->data != NULL);

  if(status < 0) {
    fprintf(stderr, "Error on connect: %d (%s)\n", status, uv_strerror(status));
    return;
  }

  rfs__client_listener_t* listener = slistener->loop->data;
  rfs__client_conn_t* conn = malloc(sizeof(rfs__client_conn_t));

  if(conn == NULL) {
    fprintf(stderr, "Unable to allocate memory\n");
    return;
  }

  uv_pipe_init(slistener->loop, &(conn->pipe), 0);
  LIST_INSERT_HEAD(&(listener->conns_head), conn, conns);
  conn->conn_time = time(NULL);
  conn->pipe.data = conn;

  int ret;
  if((ret = uv_accept(slistener, (uv_stream_t*) &(conn->pipe))) < 0) {
    fprintf(stderr, "Error on accept: %d (%s)\n", ret, uv_strerror(ret));

    rfs__client_conn_free(conn);
    return;
  }

  uv_os_fd_t cfd;

  if((ret = uv_fileno((uv_handle_t*) &(conn->pipe), &cfd)) < 0) {
    fprintf(stderr, "Error getting fileno of stream: %d (%s)\n",
            ret, uv_strerror(ret));

    rfs__client_conn_free(conn);
    return;
  }

#ifdef __APPLE__
  static const int so_lvl = SOL_LOCAL;
  static const int so_opt = LOCAL_PEERCRED;
  struct xucred cred;
#else
  static const int so_lvl = SOL_SOCKET;
  static const int so_opt = SO_PEERCRED;
  struct ucred cred;
#endif

  socklen_t clen = sizeof(cred);

  if((ret = getsockopt(cfd, so_lvl, so_opt, &cred, &clen)) < 0) {
    fprintf(stderr, "Error getting cred opt: %d (%s)\n",
            errno, strerror(errno));

    rfs__client_conn_free(conn);
    return;
  }

  if(cred.cr_uid != getuid()) {
    fprintf(stderr, "Unauthenticated connection: our uid %d, inc uid %d\n",
            getuid(), cred.cr_uid);

    rfs__client_conn_free(conn);
    return;
  }

  uv_read_start((uv_stream_t*) &(conn->pipe),
                rfs__client_alloc_buf,
                rfs__client_on_read);
}

/// @brief Start and run the worker thread.
/// This function should be invoked as the function provided to a new thread;
/// once started it will run until it receives a RFS__CLIENT_SHUTDOWN function
/// request over its listening socket. It is a blocking call, so don't expect
/// anything run after uv_run to be executed.
/// @param [in] args Unused for now.
static void rfs__client_run(void* args) {
  (void) args;

  uv_loop_t* loop = malloc(sizeof(uv_loop_t));

  if(loop == NULL) {
    // log out of memory
    return;
  }

  rfs__client_listener_t* listener = malloc(sizeof(rfs__client_listener_t));

  if(listener == NULL) {
    // log out of memory
    free(loop);
    return;
  }

  listener->path = malloc(sizeof(char) * UNIX_PATH_MAX);

  if(listener->path == NULL) {
    // log out of memory
    free(listener);
    free(loop);
    return;
  }

  uv_loop_init(loop);
  uv_pipe_init(loop, &(listener->pipe), 0);
  LIST_INIT(&(listener->conns_head));

  loop->data = listener;
  _listener = listener;

  int plen = snprintf(listener->path,
                      UNIX_PATH_MAX,
                      "/tmp/rfsct_%ld",
                      rfs__getpid());

  if(plen < 0) {
    // log error
    rfs__client_listener_free(listener);
    free(loop);
    return;
  }

  // First, we remove the path, in case it was left over
  uv_fs_t req; // this doesn't need any args for unlink to succeed
  uv_fs_unlink(loop, &req, listener->path, NULL);

  int ret;
  if((ret = uv_pipe_bind(&(listener->pipe), listener->path)) < 0) {
    fprintf(stderr, "Unable to bind to %s: %d (%s)\n",
            listener->path, ret, uv_strerror(ret));

    rfs__client_listener_free(listener);
    free(loop);
    return;
  }

  if((ret = uv_listen((uv_stream_t*) &(listener->pipe), 128,
      rfs__client_on_connect)) < 0) {
    fprintf(stderr, "Unable to listen: %d (%s)\n", ret, uv_strerror(ret));
    rfs__client_listener_free(listener);
    free(loop);
    return;
  }
  else {
    fprintf(stdout, "Listening for API calls on %s\n", listener->path);
  }

  uv_run(loop, UV_RUN_DEFAULT);
}

void rfs__client_start(void) {

  // Ignore signal events
  signal(SIGPIPE, SIG_IGN);

  // Create new thread for processing
  uv_thread_t client_thread;
  uv_thread_create(&client_thread, rfs__client_run, NULL);  

  sleep(1);
}

const char* rfs__client_get_path(void) {
  if(_listener == NULL)
    return NULL;

  return _listener->path;
}
