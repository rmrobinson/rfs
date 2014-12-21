// Sourced from http://c.learncodethehardway.org/book/ex20.html
// Modified to improve usefulness.

#ifndef RFS__LOG_H_
#define RFS__LOG_H_

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

#ifdef NDEBUG
/// @brief If debugging is disabled, we log nothing for debug.
/// @param [in] M The message formatting string.
/// @param [in] ... The arguments to the format string.
#define L_DEBUG(M, ...)
#else
/// @brief Log a debug event.
/// This convenience maco includes the file name and line number where the
/// event occurred.
/// @param [in] M The message formatting string.
/// @param [in] ... The arguments to the format string.
#define L_DEBUG(M, ...) log_it(LOG_DEBUG, M " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)
#endif

/// @brief Log an informational event.
/// @param [in] M The message formatting string.
/// @param [in] ... The arguments to the format string.
#define L_INFO(M, ...) log_it(LOG_INFO,  M "\n", ##__VA_ARGS__)

/// @brief Log an error statement.
/// If errno is set, its value will be logged as well. It also logs the file
/// name and line number where the event occurred.
/// If errno is set, its value will be logged as well.
/// @param [in] M The message formatting string.
/// @param [in] ... The arguments to the format string.
#define L_ERR(M, ...) \
do { \
  if (errno != 0) \
    log_it(LOG_ERR, M " error=%d (%s) (%s:%d)\n", ##__VA_ARGS__, errno, strerror(errno), __FILE__, __LINE__); \
  else \
    log_it(LOG_ERR, M " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__); \
} while(0)

/// @brief Log a warning statement.
/// If errno is set, its value will be logged as well. It also logs the file
/// name and line number where the event occurred.
/// @param [in] M The message formatting string.
/// @param [in] ... The arguments to the format string.
#define L_WARN(M, ...) \
do { \
  if (errno != 0) \
    log_it(LOG_WARNING, M " error=%d (%s) (%s:%d)\n", ##__VA_ARGS__, errno, strerror(errno), __FILE__, __LINE__); \
  else \
    log_it(LOG_WARNING, M " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__); \
} while(0)

/// @brief Convenience macro to ensure code isn't reached.
/// If SENTINEL is reached, it will be logged at error level, errno
/// will be reset and code will jump to cleanup.
/// @param [in] M The format string to log.
/// @param [in] ... The arguments to the format string.
#define SENTINEL(M, ...) \
do { \
  L_ERR(M, ##__VA_ARGS__); \
  errno=0; \
  goto cleanup; \
} while(0)

/// @brief Convenience macro to log and cleanup on error.
/// If the supplied condition is false, it will be logged at error level, errno
/// will be reset and code will jump to cleanup.
/// @param [in] A The condition to check for.
/// @param [in] M The format string to log.
/// @param [in] ... The arguments to the format string.
#define CHECK(A, M, ...) \
do { \
  if (!(A)) { \
    L_ERR(M, ##__VA_ARGS__); \
    errno=0; \
    goto cleanup; \
  } \
} while(0)

/// @brief Convenience macro to log and cleanup if the condition isn't true.
/// If the supplied condition is false, it will be logged at debug level, errno
/// will be reset and code will jump to cleanup.
/// @param [in] A The condition to check for.
/// @param [in] M The format string to log.
/// @param [in] ... The arguments to the format string.
#define CHECK_DEBUG(A, M, ...) \
do { \
  if (!(A)) { \
    L_DEBUG(M, ##__VA_ARGS__); \
    errno=0; \
    goto cleanup; \
  } \
} while(0)

/// @brief Convenience macro to log and cleanup if memory allocation failed.
/// If the supplied memory pointer is null, it will be logged at error level,
/// errno will be reset and code will jump to cleanup.
/// @param [in] A The memory pointer to check.
#define CHECK_MEM(A) CHECK((A), "Out of memory.")

/// @brief Initialize the logging system.
/// This should be called, once, at the beginning of the program, to configure
/// the logging behaviour.
/// @param [in] program The name of the program.
/// @param [in] console If true logs are sent to the console; else to syslog.
void log_init(const char* program, bool console);

/// @brief Log the provided message.
/// @param [in] priority The priority of the message; see syslog.h's priorities.
/// @param [in] format The output format.
/// @param [in] ... The arguments required by the format string.
void log_it(int priority, const char* format, ...);

#endif
