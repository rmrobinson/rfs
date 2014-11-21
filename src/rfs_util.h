#ifndef RFS_UTIL_H
#define RFS_UTIL_H

/// @brief Platform-independent function to retrieve the process ID.
/// @return Process ID of current process.
long int rfs__getpid(void);

/// @brief Platform-independent function to retrieve the thread ID.
/// @return Thread ID of the current thread.
long int rfs__gettid(void);

#endif

