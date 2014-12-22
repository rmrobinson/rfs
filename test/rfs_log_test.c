#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "src/log.h"

int main(int argc, char* argv[]) {
  assert(argc > 0);
  log_init(argv[0], true);

  char* tmp = NULL;
  L_DEBUG("This is a debug test of %d args: '%s', %d", 2, "a string", 3243);
  L_INFO("This is an info event with a float: %f", 3.14);
  errno = EINVAL;
  L_WARN("This is a %s event", "warning");
  errno = EBADMSG;
  L_ERR("This is an error event");

  bool test = true;

  CHECK(test, "The test is false");

  log_it(LOG_ALERT, "Alert! %s is still running\n", argv[0]);
  log_it(LOG_NOTICE, "Notice! %s is good!\n", argv[0]);

  tmp = malloc(100);
  CHECK_MEM(tmp);

  test = false;
  CHECK_DEBUG(test, "The test is false");

  SENTINEL("%s is not false!", test ? "true" : "false");

cleanup:
  if (tmp != NULL) {
    free(tmp);
    tmp = NULL;
  }

  L_INFO("We're about to exit");
  return 0;
}
