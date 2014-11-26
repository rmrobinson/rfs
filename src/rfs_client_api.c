
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "rfs/rfs.h"
#include "rfs_client.h"
#include "rfs_util.h"

/// @brief Data fields required by an accessor thread.
typedef struct rfs__client_ctx_thread {
  int sfd; ///< The socket descriptor to the worker thread.
} rfs__client_ctx_thread_t;

/// @brief The current thread's context.
/// This contains, if already initialized, the connection to the
/// worker thread.
__thread rfs__client_ctx_thread_t* _tctx = NULL;

/// @brief Initialize a connection to the worker thread.
/// @param [in] t The thread context to store the socket descriptor in.
/// @return 0 on success, -errno on failure.
static int rfs__client_init_thread_ctx(rfs__client_ctx_thread_t* t) {
  assert(t != NULL);

  const char* path = rfs__client_get_path();
  assert(path != NULL);

  if(t->sfd >= 0)
    return 0;

  struct sockaddr_un sun = { .sun_family = AF_LOCAL };
  size_t pathlen = strlen(path);

  if(pathlen > sizeof(sun.sun_path)) {
    // log error
    return -ENAMETOOLONG;
  }

  strncpy(sun.sun_path, path, pathlen);
  sun.sun_path[pathlen] = '\0';
  socklen_t slen = sizeof(sun);

  int ret = socket(AF_LOCAL, SOCK_STREAM, 0);

  if(ret < 0) {
    // log error
    return -errno;
  }

  t->sfd = ret;

  ret = connect(t->sfd, (struct sockaddr*) &sun, slen);

  if(ret < 0) {
    // log error
    close(t->sfd);
    t->sfd = -1;

    return -errno;
  }

  return 0;
}

int rfs__client_invoke(rfs__client_func_t* func) {
  assert(func != NULL);

  if(_tctx == NULL) {
    _tctx = malloc(sizeof(rfs__client_ctx_thread_t));
    assert(_tctx != NULL);

    _tctx->sfd = -1;
  }

  int ret = 0;
  if(_tctx->sfd < 0) {
    ret = rfs__client_init_thread_ctx(_tctx);

    if(ret < 0)
      return ret;
  }

  uintptr_t req = (uintptr_t) func;
  ret = send(_tctx->sfd, &req, sizeof(req), 0);

  if(ret < 0)
    return -errno;

  uintptr_t resp;
  ret = recv(_tctx->sfd, &resp, sizeof(resp), 0);

  if(ret < 0) {
    fprintf(stderr, "Error receiving response: %d (%s)\n",
            errno, strerror(errno));
    return -errno;
  }
  else if(ret == 0) {
    fprintf(stderr, "pipe closed connection\n");
    return -ECONNRESET;
  }
  else if(ret != sizeof(resp)) {
    fprintf(stderr, "pipe didn't read appropriate size: %d\n", ret);
    return -EMSGSIZE;
  }

  if(req != resp) {
    fprintf(stderr, "pipe returned different ptr\n");
    return -EBADMSG;
  }

  return 0;
}

void rfs_init(void) {
  rfs__client_start();
}

void rfs_deinit(void) {
  rfs__client_func_t func;
  func.type = RFS__CLIENT_SHUTDOWN;

  rfs__client_invoke(&func);
}

int rfs_bind(const char* name, const char* old, int flags) {
  rfs__client_func_t func;
  func.type = RFS__CLIENT_FUNC_BIND;
  func.args.bind.name = name;
  func.args.bind.old = old;
  func.args.bind.flags = flags;

  int ret = rfs__client_invoke(&func);

  return (ret == 0 ? func.ret : ret);
}

int rfs_mount(int fd,
              rfs_fd_t afd,
              const char* old,
              int flags,
              const char* aname) {
  rfs__client_func_t func;
  func.type = RFS__CLIENT_FUNC_MOUNT;
  func.args.mount.fd = fd;
  func.args.mount.afd = afd;
  func.args.mount.old = old;
  func.args.mount.flags = flags;
  func.args.mount.aname = aname;

  int ret = rfs__client_invoke(&func);

  return (ret == 0 ? func.ret : ret);
}

int rfs_unmount(const char* name, const char* old) {
  rfs__client_func_t func;
  func.type = RFS__CLIENT_FUNC_UNMOUNT;
  func.args.unmount.name = name;
  func.args.unmount.old = old;

  int ret = rfs__client_invoke(&func);

  return (ret == 0 ? func.ret : ret);
}

