
#ifndef RFS_PROTO_9P
#define RFS_PROTO_9P

#include "rfs/types.h"

/// @file The Plan 9 wire protocol functions.
/// The types and functions to serialize and deserialize P9 messages.
/// This should not be used directly, it is wrapped by rfs_9p.h which exposes
/// more user-friendly functions. 
/// @todo Greatly improve the implementation of the pack/unpack functions.
/// They assert when they should return errors, they are likely vulnerable
/// to overflow attacks, etc. This implementation is the bare minimum
/// for a trusted, development network, not the real world.

// Per Plan9's fcall.h, R-types are one greater than the corresponding T-types.
enum {
  RFS__9P_TVERSION = 100,
  RFS__9P_RVERSION,

  RFS__9P_TAUTH = 102,
  RFS__9P_RAUTH,

  RFS__9P_TERROR = 106, // invalid, defined for consistency with fcall.h
  RFS__9P_RERROR,

  RFS__9P_TFLUSH = 108,
  RFS__9P_RFLUSH,

  RFS__9P_TATTACH = 104,
  RFS__9P_RATTACH,

  RFS__9P_TWALK = 110,
  RFS__9P_RWALK,

  RFS__9P_TOPEN = 112,
  RFS__9P_ROPEN,

  RFS__9P_TCREATE = 114,
  RFS__9P_RCREATE,

  RFS__9P_TREAD = 116,
  RFS__9P_RREAD,

  RFS__9P_TWRITE = 118,
  RFS__9P_RWRITE,

  RFS__9P_TCLUNK = 120,
  RFS__9P_RCLUNK,

  RFS__9P_TREMOVE = 122,
  RFS__9P_RREMOVE,

  RFS__9P_TSTAT = 124,
  RFS__9P_RSTAT,

  RFS__9P_TWSTAT = 126,
  RFS__9P_RWSTAT
};

// maximum number of values to return in 1 walk request
#define RFS__9P_MAXWELEM          16

// invalid tag
#define RFS__9P_NOTAG             (uint16_t) ~0U

// invalid fid
#define RFS__9P_NOFID             (uint32_t) ~0U

typedef struct rfs__9p_stat {
  uint16_t size;
  uint16_t type;
  uint32_t dev;
  rfs_qid_t qid;
  uint32_t mode;
  uint32_t atime;
  uint32_t mtime;
  uint64_t length;
  char* name;
  char* uid;
  char* gid;
  char* muid;
} rfs__9p_stat_t;

typedef struct rfs__9p_msg {
  uint32_t size;

  uint8_t type; // see the RFS__9P enum values above for valid values.

  uint16_t tag;

  union {
    struct {
      uint32_t msize;
      char* version;
    } version;

    struct {
      uint32_t afid;
      char* uname;
      char* aname;
    } tauth;

    struct {
      rfs_qid_t aqid;
    } rauth;

    struct {
      char* ename;
    } rerror;

    struct {
      uint16_t oldtag;
    } tflush;

    // rflush is empty.

    struct {
      uint32_t fid;
      uint32_t afid;
      char* uname;
      char* aname;
    } tattach;

    struct {
      rfs_qid_t qid;
    } rattach;

    struct {
      uint32_t fid;
      uint32_t newfid;
      uint16_t nwname;
      char* wname[RFS__9P_MAXWELEM];
    } twalk;

    struct {
      uint16_t nwqid;
      rfs_qid_t wqid[RFS__9P_MAXWELEM];
    } rwalk;

    struct {
      uint32_t fid;
      uint8_t mode;
    } topen;

    struct {
      rfs_qid_t qid;
      uint32_t iounit;
    } ropen, rcreate;

    struct {
      uint32_t fid;
      char* name;
      uint32_t perm;
      uint8_t mode;
    } tcreate;

    struct {
      uint32_t fid;
      uint64_t offset;
      uint32_t count;
    } tread;

    struct {
      uint32_t count;
      unsigned char* data;
    } rread;

    struct {
      uint32_t fid;
      uint64_t offset;
      uint32_t count;
      unsigned char* data;
    } twrite;

    struct {
      uint32_t count;
    } rwrite;

    struct {
      uint32_t fid;
    } tclunk, tremove, tstat;

    // rclunk, rremove are empty

    struct {
      rfs__9p_stat_t* stat;
    } rstat;

    struct {
      uint32_t fid;
      rfs__9p_stat_t* stat;
    } twstat;

    // rwstat is empty
  } params;

} rfs__9p_msg_t;

/// @brief Initializes a stat structure.
/// @note This will not free any allocated strings!
/// @param [in] stat The structure to initialize.
void rfs__9p_stat_init(rfs__9p_stat_t* stat);

/// @brief Resets all values to initial, freeing any allocated strings.
/// @note Do not call on strings you cannot free - i.e. from stat_unpack.
/// @param [in] stat The structure to reset.
void rfs__9p_stat_reset(rfs__9p_stat_t* stat);

/// @brief Calculates the serialized size of a stat structure.
/// @param [in] stat The structure to calculate the size of.
/// @return The size of the structure, if serialized.
uint16_t rfs__9p_stat_size(const rfs__9p_stat_t* stat);

/// @brief Serialize the stat structure to the provided buffer.
/// @param [in] stat The structure to serialize.
/// @param [in] buf The buffer to serialize the stat to.
/// @param [in] bufsize The maximum amount of data which can be written.
/// @return The number of bytes in buf used; 0 on error.
size_t rfs__9p_stat_pack(rfs__9p_stat_t* stat,
                         unsigned char* buf,
                         size_t bufsize);

/// @brief Deserializes a stat structure from the provided buffer.
/// @param [in] buf The buffer to retrieve the data from.
/// This is modified so that the stat structure can reference included strings.
/// @param [in] bufsize The size of the buffer.
/// @param [in] stat The structure to deserialize into.
/// @return The number of bytes processed from buf; 0 on error.
size_t rfs__9p_stat_unpack(unsigned char* buf,
                           size_t bufsize,
                           rfs__9p_stat_t* stat);

/// @brief Initializes a message structure.
/// @note This will not free any allocated strings!
/// @param [in] msg The structure to initialize.
void rfs__9p_msg_init(rfs__9p_msg_t* msg);

/// @brief Resets all values to initial, freeing any allocated strings.
/// @note Do not call on strings you cannot free - i.e. from msg_unpack.
/// @param [in] msg The structure to reset.
void rfs__9p_msg_reset(rfs__9p_msg_t* msg);

/// @brief Calculates the serialized size of a msg structure.
/// @param [in] msg The structure to calculate the size of.
/// @return The size of the structure, if serialized.
uint32_t rfs__9p_msg_size(const rfs__9p_msg_t* msg);

/// @brief Serialize the msg structure to the provided buffer.
/// @param [in] msg The structure to serialize.
/// @param [in] buf The buffer to serialize the stat to.
/// @param [in] bufsize The maximum amount of data which can be written.
/// @return The number of bytes in buf used; 0 on error.
size_t rfs__9p_msg_pack(rfs__9p_msg_t* msg,
                        unsigned char* buf,
                        size_t bufsize);

/// @brief Deserializes a message structure from the provided buffer.
/// @param [in] buf The buffer to retrieve the data from.
/// This is modified so that the msg structure can reference included strings.
/// @param [in] bufsize The size of the buffer.
/// @param [in] msg The structure to deserialize into.
/// @return The number of bytes processed from buf; 0 on error.
size_t rfs__9p_msg_unpack(unsigned char* buf,
                          size_t bufsize,
                          rfs__9p_msg_t* msg);

#endif

