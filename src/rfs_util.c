#include <pthread.h>
#include <unistd.h>

#include "rfs_util.h"

long int rfs__getpid(void) {
  return (long int) getpid();
}

long int rfs__gettid(void) {
#ifdef __APPLE__
  return (long int) pthread_mach_thread_np(pthread_self());
#else
  return (long int) pthread_getthreadid_np();
#endif
}
