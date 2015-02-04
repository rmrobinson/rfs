#include <pthread.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/syscall.h>
#endif

#include "rfs_util.h"

long int rfs__getpid(void) {
  return (long int) getpid();
}

long int rfs__gettid(void) {
#ifdef __APPLE__
  return (long int) pthread_mach_thread_np(pthread_self());
#elif defined(__linux__)
  return (long int) syscall(__NR_gettid);
#else
  return (long int) pthread_getthreadid_np();
#endif
}

