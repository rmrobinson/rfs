#ifndef RFS_TYPES_H
#define RFS_TYPES_H

#include <stddef.h>
#include <stdint.h>

enum {
  RFS_QTDIR = 0x80,
  RFS_QTAPPEND = 0x40,
  RFS_QTEXCL = 0x20,
  RFS_QTMOUNT = 0x10,
  RFS_QTAUTH = 0x08,
  RFS_QTTMP = 0x04,
  RFS_QTFILE = 0x00
};

typedef struct rfs_qid {
  uint64_t path;
  uint32_t vers;
  uint8_t type;
} rfs_qid_t;

#define  RFS_DMDIR        = 0x80000000,
#define  RFS_DMAPPEND     = 0x40000000,
#define  RFS_DMEXCL       = 0x20000000,
#define  RFS_DMMOUNT      = 0x10000000,
#define  RFS_DMAUTH       = 0x08000000,
#define  RFS_DMTMP        = 0x04000000,
#define  RFS_DMREAD       = 0x4,
#define  RFS_DMWRITE      = 0x2,
#define  RFS_DMEXEC       = 0x1

typedef struct rfs_dirent {
  uint16_t type;
  uint32_t dev;
  rfs_qid_t qid;
  uint32_t mode;
  uint32_t atime;
  uint32_t mtime;
  uint64_t length;
  char* name; // last element of the path
  char* uid;
  char* gid;
  char* muid;
} rfs_dirent_t;

#endif

