#ifndef RFS_RFS_H
#define RFS_RFS_H

#include "rfs/types.h"

enum {
  RFS_MREPL    = (1 << 0), ///< Replace old file with new one
  RFS_MBEFORE  = (1 << 1), ///< Mount at the start of the search order.
  RFS_MAFTER   = (1 << 2), ///< Mount at the end of the search order.
  RFS_MCREATE  = (1 << 3), ///< A mount which files can be created on.
  RFS_MCACHE   = (1 << 4)  ///< Cache content at client.
};

void rfs_init(void);

void rfs_deinit(void);

int rfs_bind(const char* name, const char* old, int flags);

int rfs_mount(int fd,
              rfs_fd_t afd,
              const char* old,
              int flag,
              const char* aname);

int rfs_unmount(const char* name, const char* old);


#endif

