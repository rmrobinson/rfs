#ifndef RFS_TYPES_H
#define RFS_TYPES_H

#include <stddef.h>
#include <stdint.h>

/// @brief These enum values describe the possible bitmask values of
/// the 'type' field in a QID.
enum {
  RFS_QTDIR = 0x80, ///< This is a directory.
  RFS_QTAPPEND = 0x40, ///< This file is append-only.
  RFS_QTEXCL = 0x20, ///< The file is is marked as exclusive use only.
  RFS_QTMOUNT = 0x10, ///< This is a mounted channel.
  RFS_QTAUTH = 0x08, ///< This is an authentication file.
  RFS_QTTMP = 0x04, ///< This file is not backed up.
  RFS_QTFILE = 0x00 ///< Regular file.
};

/// @brief An ID which is unique across all files on a server.
/// man 2 stat for more details.
typedef struct rfs_qid {
  /// @brief Each file has a path, which is unique per server.
  /// It will not change, regardless of moves or renames.
  /// 2 files are only the same if their paths are equal.
  uint64_t path;

  /// @brief The current version of the file.
  uint32_t vers;

  /// @brief The type of this file, see the RFS_QT* values.
  uint8_t type;
} rfs_qid_t;

// These are not in an enum because RFS_DMDIR is too large to fit in an int,
// which is the assumed size of an enum in C.

/// @brief The entity is a directory.
#define  RFS_DMDIR        = 0x80000000,
/// @brief The entity is an append-only file.
#define  RFS_DMAPPEND     = 0x40000000,
/// @brief The entity is an exclusive use file.
#define  RFS_DMEXCL       = 0x20000000,
/// @brief The entity is a mounted channel.
#define  RFS_DMMOUNT      = 0x10000000,
/// @brief The entity is an authentication file.
#define  RFS_DMAUTH       = 0x08000000,
/// @brief The entity is not backed up.
#define  RFS_DMTMP        = 0x04000000,
/// @brief The read permission bit.
#define  RFS_DMREAD       = 0x4,
/// @brief The write permission bit..
#define  RFS_DMWRITE      = 0x2,
/// @brief The execute permission bit.
#define  RFS_DMEXEC       = 0x1

/// @brief A directory entity. man 2 stat for more details.
typedef struct rfs_dirent {
  // These fields don't relate directly to the dirent.
  uint16_t type; ///< The type of the server hosting this file.
  uint32_t dev; ///< The subtype of the server hosting this file.

  // These fields are file-specific.
  /// @brief The unique ID of the dirent on the server.
  rfs_qid_t qid;
  /// @brief The bitwise OR of RFS_DM* values defined for this dirent.
  uint32_t mode;
  /// @brief The last time this dirent was accessed. Seconds since epoch.
  uint32_t atime;
  /// @brief The last time this dirent was modified. Seconds since epoch.
  uint32_t mtime;
  /// @brief The length of the dirent.
  /// 0 for directories.
  /// Number of bytes in a file for files.
  /// Number of bytes to read before blocking for streams.
  uint64_t length;
  /// @brief The last element of the path.
  char* name;
  /// @brief The name of the owner of the file.
  char* uid;
  /// @brief The name of the group of the file.
  char* gid;
  /// @brief Name of the user who last modified the file (i.e. updated mtime).
  char* muid;
} rfs_dirent_t;

/// @brief The type used to represent a file descriptor in rfs.
typedef int rfs_fd_t;

#endif

