#include <stdio.h>
#include <unistd.h>

#include "rfs/rfs.h"

int main() {
  rfs_init();
  int ret = rfs_bind("file a", "file b", 0);
  fprintf(stdout, "rfs_bind returned %d\n", ret);
  rfs_deinit();
 
  return 0;
}

