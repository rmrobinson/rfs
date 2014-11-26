#ifndef RFS_CLIENT_H
#define RFS_CLIENT_H

#include "rfs/types.h"

/// @brief Different function calls supported across the RFS client channel.
/// The RFS client API can be accessed by multiple threads, and these requests
/// are serialized across a local socket to a single worker thread.
/// These are the supported API calls.
typedef enum rfs__client_func_type {
  RFS__CLIENT_FUNC_BIND = 1, ///< bind()
  RFS__CLIENT_FUNC_MOUNT = 2, ///< mount()
  RFS__CLIENT_FUNC_UNMOUNT = 3, ///< unmount()
  RFS__CLIENT_SHUTDOWN = 254 ///< shut down the worker thread.
} rfs__client_func_type_t;

/// @brief A function, 'serialized' across the local socket.
/// This function structure refers to pointers managed by the calling thread.
/// This is safe because the calling thread is blocked waiting for the
/// worker thread to respond. However, this is NOT SAFE for use in an async
/// program structure.
/// The function call is made by setting the type and the appropriate fields
/// in the struct associated with that function type. Upon completion, the
/// ret field is set and the structure is passed back.
typedef struct rfs__client_func {
  int ret; ///< The return code from this function's execution.

  rfs__client_func_type_t type; ///< The type of function being executed.

  /// @brief Documentation of the behaviour of each argument to each of the
  /// structs in args can be found in the rfs/rfs.h header file (there is a
  /// 1:1 mapping between function calls and function argments in that header
  /// and this collection of structures.
  union {
    struct {
      const char* name;
      const char* old;
      int flags;
    } bind;

    struct {
      int fd;
      rfs_fd_t afd;
      const char* old;
      int flags;
      const char* aname;
    } mount;

    struct {
      const char* name;
      const char* old;
    } unmount;
  } args;
} rfs__client_func_t;

/// @brief Execute the function request.
/// This function can be called by any thread (except for the worker thread).
/// If there is no active  connection from the current thread to the worker
/// thread it will create one, then pass this request to the worker thread.
/// @param [in] func The function request structure to invoke.
/// @return 0 on success, -errno on failure.
int rfs__client_invoke(rfs__client_func_t* func);

/// @brief Start the RFS client worker thread.
void rfs__client_start(void);

/// @brief Exposes in a thread-safe way the worker thread's local socket path.
/// This path is used by calls from accessor threads to initialize a connection
/// to the worker thread.
/// @todo Support abstract sockets on Linux (requires len field)
/// @return NULL if the processing thread isn't initialized; local socket path
/// the processing thread is listening on once it is started.
const char* rfs__client_get_path(void);

#endif

