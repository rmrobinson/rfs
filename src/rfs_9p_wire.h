
#ifndef RFS_9P_WIRE_H
#define RFS_9P_WIRE_H

#include "rfs/types.h"

/// @file The Plan 9 wire protocol functions.
/// The types and functions to serialize and deserialize P9 messages.
/// This should not be used directly, it is wrapped by rfs_9p.h which exposes
/// more user-friendly functions. 
///
/// Per the 9P spec, all multi-byte integer values are encoded as little
/// endian values. Strings are prefixed with a 2-byte size, and strings
/// are not null terminated.
///
/// @todo Greatly improve the implementation of the pack/unpack functions.
/// They assert when they should return errors, they are likely vulnerable
/// to overflow attacks, etc. This implementation is the bare minimum
/// for a trusted, development network, not the real world.

/// @brief Per Plan9's fcall.h, R-types are one greater than their T-types.
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

/// @brief The maximum number of values to return in 1 walk request
#define RFS__9P_MAXWELEM          16

/// @brief An invalid tag
#define RFS__9P_NOTAG             (uint16_t) ~0U

/// @brief An invalid fid
#define RFS__9P_NOFID             (uint32_t) ~0U

/// @brief The struct which stat data will be serialized to/from.
/// Users transmitting this field should fill in everything except for
/// size (which will be overwritten during serialization), then call
/// rfs__9p_stat_pack() to serialize this object.
/// Users receiving this field should obtain the buffer, then call
/// rfs__9p_stat_unpack() to deserialize the buffer into this struct.
///
/// See man 5 stat for additional details.
typedef struct rfs__9p_stat {
  uint16_t size; ///< Total size of the stat, this field included.
  uint16_t type; ///< The server type.
  uint32_t dev; ///< The server sub-type.
  rfs_qid_t qid; ///< The unique identifier of this entity on the server.
  uint32_t mode; ///< The permission bits and flags.
  uint32_t atime; ///< The last access time, seconds since epoch.
  uint32_t mtime; ///< The last modification time, seconds since epoch.
  uint64_t length; ///< The length of the file, in bytes.
  char* name; ///< The last entry in the path. Must be / for root directories.
  char* uid; ///< Name of the owner.
  char* gid; ///< Name of the group.
  char* muid; ///< Name of the last user to modify the file.
} rfs__9p_stat_t;

/// @brief The structure which 9P messages will be serialized to/from.
/// Users transmitting this field should fill in the type, and the appropriate
/// param union value, prior to calling rfs__9p_msg_pack() to serialize it.
/// Users receiving this field should obtain the buffer, then call
/// rfs__9p_msg_unpack() to deserialize the message into this struct.
///
/// See man 5 intro and man 2 fcall for more details.
typedef struct rfs__9p_msg {
  /// @brief The size of the message, this field included.
  uint32_t size;

  /// @brief The type of this message. See RFS__9P* for valid values.
  uint8_t type;

  /// @brief The identifying tag of this message.
  uint16_t tag;

  union {
    struct {
      uint32_t msize; ///< The maximum supported size of the message.
      char* version; ///< The version string.
    } version; ///< The content of the T and Rversion messages.

    struct {
      uint32_t afid; ///< The authentication fid.
      char* uname; ///< The username of the auth request.
      char* aname; ///< The name of the file tree to be authenticating to.
    } tauth; ///< The content of the Tauth message.

    struct {
      rfs_qid_t aqid; ///< The qid of the authentication file.
    } rauth; ///< The content of the Rauth message.

    struct {
      char* ename; ///< The error string describing why a request failed.
    } rerror; ///< The content of the Rerror message.

    struct {
      uint16_t oldtag; ///< The tag of the message to abort.
    } tflush; ///< The content of the Tflush message.

    // rflush is empty.

    struct {
      uint32_t fid; ///< The fid to save as the root of the file system.
      uint32_t afid; ///< The previously obtained auth fid from attach.
      char* uname; ///< The name of the user to attach as.
      char* aname; ///< The name of the file tree to access.
    } tattach; ///< The content of the Tattach message.

    struct {
      rfs_qid_t qid; ///< The qid of the root of the file tree.
    } rattach; ///< The content of the Rattach message.

    struct {
      uint32_t fid; ///< The fid to start the walk from.
      uint32_t newfid; ///< The fid to represent the result as.
      uint16_t nwname; ///< The number of elements in the walk request.
      char* wname[RFS__9P_MAXWELEM]; ///< The path elements to walk.
    } twalk; ///< The content of the Twalk message.

    struct {
      uint16_t nwqid; ///< The number of qid values.
      rfs_qid_t wqid[RFS__9P_MAXWELEM]; ///< The path element results.
    } rwalk; ///< The content of the Rwalk message.

    struct {
      uint32_t fid; ///< The fid to open.
      uint8_t mode; ///< The mode to open the file with.
    } topen; ///< The content of the Topen message.

    struct {
      rfs_qid_t qid; ///< The qid of the file opened.
      uint32_t iounit; ///< The max # of bytes tx w/out splitting the msg.
    } ropen, rcreate; /// The content of the Ropen and Rcreate messages.

    struct {
      uint32_t fid; ///< The fid of the directory to create the file in.
      char* name; ///< The name of the file to create (last entry in path).
      uint32_t perm; ///< The permissions to give to the file.
      uint8_t mode; ///< The mode to open the file with.
    } tcreate; ///< The content of the Tcreate message.

    struct {
      uint32_t fid; ///< The fid of the file to read.
      uint64_t offset; ///< The offset to read from in the file.
      uint32_t count; ///< The number of bytes to read.
    } tread; ///< The content of the Tread message.

    struct {
      uint32_t count; ///< The number of bytes read.
      unsigned char* data; ///< The data read from the file.
    } rread; ///< The contents of the Rread message.

    struct {
      uint32_t fid; ///< The fid of the file to write.
      uint64_t offset; ///< The offset of the file to write as.
      uint32_t count; ///< The number of bytes to write.
      unsigned char* data; ///< The data to write to the file.
    } twrite; ///< The contents of the Twrite message.

    struct {
      uint32_t count; ///< The number of bytes written.
    } rwrite; ///< The contents of the Rwrite message.

    struct {
      uint32_t fid; ///< The fid of the entry to clunk, remove or stat.
    } tclunk, tremove, tstat; ///< The contents of Tclunk, Tremove and Tstat.

    // rclunk, rremove are empty

    struct {
      rfs__9p_stat_t* stat; ///< The metadata of the stat'd file.
    } rstat; ///< The contents of the Rstat message.

    struct {
      uint32_t fid; ///< The fid of the entry to update the metadata of.
      rfs__9p_stat_t* stat; ///< The updated metadata of the entry.
    } twstat; ///< The contents of the Twstat message.

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

