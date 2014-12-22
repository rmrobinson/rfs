
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

/// @brief The output stream to log to.
/// If not set, log to syslog.
static FILE* _stream = NULL;

/// @brief Generate the log prefix for the current priority.
/// @param [in] priority The priority (see syslog's priorities) to generate
/// the log prefix string for.
/// @return The log prefix string; colourized if output to the console.
static const char* prefix(int priority) {
  if (_stream && isatty(fileno(_stream))) {
    switch (priority) {
      case LOG_EMERG:    return "\x1b[1;37;41m[EMRG]\x1b[0m";
      case LOG_ALERT:    return "\x1b[1;37;41m[ALRT]\x1b[0m";
      case LOG_CRIT:     return "\x1b[1;37;41m[CRIT]\x1b[0m";
      case LOG_ERR:      return "\x1b[1;31m[ ERR]\x1b[0m";
      case LOG_WARNING:  return "\x1b[1;33m[WARN]\x1b[0m";
      case LOG_NOTICE:   return "\x1b[1;32m[NOTC]\x1b[0m";
      case LOG_INFO:     return "\x1b[1;34m[INFO]\x1b[0m";
      case LOG_DEBUG:    return "\x1b[1;30m[ DBG]\x1b[0m";
      default:           return "[UNKN]";
    }
  }
  else {
    switch (priority) {
      case LOG_EMERG:    return "[EMRG]";
      case LOG_ALERT:    return "[ALRT]";
      case LOG_CRIT:     return "[CRIT]";
      case LOG_ERR:      return "[ ERR]";
      case LOG_WARNING:  return "[WARN]";
      case LOG_NOTICE:   return "[NOTC]";
      case LOG_INFO:     return "[INFO]";
      case LOG_DEBUG:    return "[ DBG]";
      default:           return "[UNKN]";
    }
  }
}

/// @brief Generate the current timestamp in zulu time.
/// @note This is not threadsafe.
/// @return The formatted current time.
static char* timestamp() {
  time_t now;
  time(&now);
  static char buf[sizeof("2011-10-08T07:07:09Z")];
  strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
  return buf;
}

void log_init(const char* program, bool to_console) {
  if (to_console) {
    _stream = stdout;
  }
  else {
    openlog(program, LOG_PID | LOG_NDELAY, LOG_DAEMON);
  }
}

void log_it(int priority, const char* format, ...) {
  va_list ap;

  va_start(ap, format);

  if (_stream) {
    fprintf(_stream, "%s %s ", timestamp(), prefix(priority));
    vfprintf(_stream, format, ap);
  }
  else {
    vsyslog(priority, format, ap);
  }

  va_end(ap);
}

