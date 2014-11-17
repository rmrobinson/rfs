#include "rfs_9p_wire.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline size_t uint8_pack(uint8_t val,
                                unsigned char* buf,
                                size_t bufsize) {
  if(sizeof(val) > bufsize)
    return 0;

  buf[0] = val;
  return sizeof(val);
}

static inline size_t uint8_unpack(const unsigned char* buf,
                                  size_t bufsize,
                                  uint8_t* val) {
  if(sizeof(*val) > bufsize)
    return 0;

  *val = (uint8_t) buf[0];
  return sizeof(*val);
}

static inline size_t uint16_pack(uint16_t val,
                                 unsigned char* buf,
                                 size_t bufsize) {
  if(sizeof(val) > bufsize)
    return 0;

  buf[0] = (0xFF & val);
  buf[1] = (0xFF & (val >> 8));
  return sizeof(val);
}

static inline size_t uint16_unpack(const unsigned char* buf,
                                   size_t bufsize,
                                   uint16_t* val) {
  if(sizeof(*val) > bufsize )
    return 0;

  *val = (uint8_t) buf[0];
  *val |= ((uint8_t) buf[1]) << 8;
  return sizeof(*val);
}

static inline size_t uint32_pack(uint32_t val,
                                 unsigned char* buf,
                                 size_t bufsize) {
  if(sizeof(val) > bufsize)
    return 0;

  buf[0] = (0xFF & val);
  buf[1] = (0xFF & (val >> 8));
  buf[2] = (0xFF & (val >> 16));
  buf[3] = (0xFF & (val >> 24));
  return sizeof(val);
}

static inline size_t uint32_unpack(const unsigned char* buf,
                                   size_t bufsize,
                                   uint32_t* val) {
  if(sizeof(*val) > bufsize)
    return 0;

  *val = (uint8_t) buf[0];
  *val |= ((uint8_t) buf[1]) << 8;
  *val |= ((uint8_t) buf[2]) << 16;
  *val |= ((uint8_t) buf[3]) << 24;
  return sizeof(*val);
}

static inline size_t uint64_pack(uint64_t val,
                                 unsigned char* buf,
                                 size_t bufsize) {
  if(sizeof(val) > bufsize)
    return 0;

  buf[0] = (0xFF & val);
  buf[1] = (0xFF & (val >> 8));
  buf[2] = (0xFF & (val >> 16));
  buf[3] = (0xFF & (val >> 24));
  buf[4] = (0xFF & (val >> 32));
  buf[5] = (0xFF & (val >> 40));
  buf[6] = (0xFF & (val >> 48));
  buf[7] = (0xFF & (val >> 56));
  return sizeof(val);
}

static inline size_t uint64_unpack(const unsigned char* buf,
                                   size_t bufsize,
                                   uint64_t* val) {
  if(sizeof(*val) > bufsize)
    return 0;

  *val = (uint8_t) buf[0];
  *val |= ((uint8_t) buf[1]) << 8;
  *val |= ((uint8_t) buf[2]) << 16;
  *val |= ((uint8_t) buf[3]) << 24;
  *val |= (uint64_t)((uint8_t) buf[4]) << 32;
  *val |= (uint64_t)((uint8_t) buf[5]) << 40;
  *val |= (uint64_t)((uint8_t) buf[6]) << 48;
  *val |= (uint64_t)((uint8_t) buf[7]) << 56;
  return sizeof(*val);
}

static inline size_t qid_size() {
  return (sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint64_t));
}

static inline size_t qid_pack(const rfs_qid_t* val,
                              unsigned char* buf,
                              size_t bufsize) {
  if(qid_size() > bufsize)
    return 0;

  size_t used = 0;
  used += uint8_pack(val->type, buf + used, bufsize - used);
  used += uint32_pack(val->vers, buf + used, bufsize - used);
  used += uint64_pack(val->path, buf + used, bufsize - used);
  return used;
}

static inline size_t qid_unpack(const unsigned char* buf,
                                size_t bufsize,
                                rfs_qid_t* val) {
  if(qid_size() > bufsize)
    return 0;

  size_t used = 0;
  used += uint8_unpack (buf + used, bufsize - used, &(val->type));
  used += uint32_unpack(buf + used, bufsize - used, &(val->vers));
  used += uint64_unpack(buf + used, bufsize - used, &(val->path));
  return used;
}

static inline size_t str_size(const char* val) {
  if(!val)
    return sizeof(uint16_t);

  return sizeof(uint16_t) + strlen(val);
}

static inline size_t str_pack(const char* val,
                              unsigned char* buf,
                              size_t bufsize) {
  if(!val)
    return uint16_pack(0, buf, bufsize);

  size_t slen = strlen(val);

  if(sizeof(uint16_t) + slen > bufsize)
    return 0;

  size_t ret = 0;

  ret += uint16_pack(slen, buf, bufsize);
  strncpy((char*) buf + ret, val, slen);
  ret += slen;

  return ret;
}

// this is a function which will not return the same result after repeated calls!
static inline size_t str_unpack(unsigned char* buf,
                                size_t bufsize,
                                char** val) {
  if(sizeof(uint16_t) > bufsize)
    return 0;

  uint16_t slen;
  assert(uint16_unpack(buf, bufsize, &slen) == sizeof(uint16_t));

  if(sizeof(uint16_t) + slen > bufsize)
    return 0;

  if(slen < 1)
    return sizeof(uint16_t);

  // here we move the string 'down' 1 byte, so that we can use the last byte
  // as the terminating null.
  unsigned char* str = buf + sizeof(uint16_t) - 1;

  // the string starts at (buf + 2), will be moved to (buf + 1)
  memmove(str, buf + sizeof(uint16_t), slen);

  str[slen] = '\0';
  *val = (char*) str;

  return sizeof(uint16_t) + slen;
}

void rfs__9p_stat_init(rfs__9p_stat_t* stat) {
  assert(stat != NULL);

  stat->size = 0;
  stat->type = 0;
  stat->dev = 0;
  rfs_qid_t q = { .path = 0, .vers = 0, .type = 0};
  stat->qid = q;
  stat->mode = 0;
  stat->atime = 0;
  stat->mtime = 0;
  stat->length = 0;
  stat->name = NULL;
  stat->uid = NULL;
  stat->gid = NULL;
  stat->muid = NULL;
}

void rfs__9p_stat_reset(rfs__9p_stat_t* stat) {
  assert(stat != NULL);

  free(stat->name);

  free(stat->uid);

  free(stat->gid);

  free(stat->muid);

  rfs__9p_stat_init(stat);
}

uint16_t rfs__9p_stat_size(const rfs__9p_stat_t* stat) {
  assert(stat != NULL);
  assert((str_size(stat->name)
        + str_size(stat->uid)
        + str_size(stat->gid)
        + str_size(stat->muid))
        < (UINT16_MAX
         - sizeof(uint16_t)
         - sizeof(uint16_t)
         - sizeof(uint32_t)
         - qid_size()
         - sizeof(uint32_t)
         - sizeof(uint32_t)
         - sizeof(uint32_t)
         - sizeof(uint64_t)));

  uint16_t size = 0;

  size += sizeof(uint16_t); // size
  size += sizeof(uint16_t); // type
  size += sizeof(uint32_t); // dev
  size += qid_size(); // qid
  size += sizeof(uint32_t); // mode
  size += sizeof(uint32_t); // atime
  size += sizeof(uint32_t); // mtime
  size += sizeof(uint64_t); // length
  size += str_size(stat->name);
  size += str_size(stat->uid);
  size += str_size(stat->gid);
  size += str_size(stat->muid);

  return size;
} 

size_t rfs__9p_stat_pack(rfs__9p_stat_t* stat,
                         unsigned char* buf,
                         size_t bufsize) {
  assert(stat != NULL);
  assert(buf != NULL);

  stat->size = rfs__9p_stat_size(stat);

  if(stat->size > bufsize)
    return 0;

  size_t used = 0;
  used += uint16_pack(stat->size, buf + used, bufsize - used);
  used += uint16_pack(stat->type, buf + used, bufsize - used);
  used += uint32_pack(stat->dev, buf + used, bufsize - used);
  used += qid_pack   (&(stat->qid), buf + used, bufsize - used);
  used += uint32_pack(stat->mode, buf + used, bufsize - used);
  used += uint32_pack(stat->atime, buf + used, bufsize - used);
  used += uint32_pack(stat->mtime, buf + used, bufsize - used);
  used += uint64_pack(stat->length, buf + used, bufsize - used);
  used += str_pack   (stat->name, buf + used, bufsize - used);
  used += str_pack   (stat->uid, buf + used, bufsize - used);
  used += str_pack   (stat->gid, buf + used, bufsize - used);
  used += str_pack   (stat->muid, buf + used, bufsize - used);

  assert(used == stat->size);
  assert(used <= bufsize);

  return used;
}

size_t rfs__9p_stat_unpack(unsigned char* buf,
                           size_t bufsize,
                           rfs__9p_stat_t* stat) {
  assert(buf != NULL);
  assert(stat != NULL);

  // release any previously contained data
  rfs__9p_stat_reset(stat);

  size_t used = 0;
  used += uint16_unpack(buf + used, bufsize - used, &(stat->size));
  used += uint16_unpack(buf + used, bufsize - used, &(stat->type));
  used += uint32_unpack(buf + used, bufsize - used, &(stat->dev));
  used += qid_unpack   (buf + used, bufsize - used, &(stat->qid));
  used += uint32_unpack(buf + used, bufsize - used, &(stat->mode));
  used += uint32_unpack(buf + used, bufsize - used, &(stat->atime));
  used += uint32_unpack(buf + used, bufsize - used, &(stat->mtime));
  used += uint64_unpack(buf + used, bufsize - used, &(stat->length));
  used += str_unpack   (buf + used, bufsize - used, &(stat->name));
  used += str_unpack   (buf + used, bufsize - used, &(stat->uid));
  used += str_unpack   (buf + used, bufsize - used, &(stat->gid));
  used += str_unpack   (buf + used, bufsize - used, &(stat->muid));

  return used;
}

void rfs__9p_msg_init(rfs__9p_msg_t* msg) {
  assert(msg != NULL);

  msg->size = 0;
  msg->type = 0;
  msg->tag = 0;

  memset(&(msg->params), 0, sizeof(msg->params));
}

void rfs__9p_msg_reset(rfs__9p_msg_t* msg) {
  assert(msg != NULL);

  switch(msg->type) {
    case RFS__9P_TVERSION:
    case RFS__9P_RVERSION:
      free(msg->params.version.version);
      break;

    case RFS__9P_TAUTH:
      free(msg->params.tauth.uname);
      free(msg->params.tauth.aname);
      break;

    case RFS__9P_RERROR:
      free(msg->params.rerror.ename);
      break;

    case RFS__9P_TATTACH:
      free(msg->params.tattach.uname);
      free(msg->params.tattach.aname);
      break;

    case RFS__9P_TWALK:
      for(uint16_t i = 0; i < msg->params.twalk.nwname; ++i) {
        free(msg->params.twalk.wname[i]);
      }
      break;

    case RFS__9P_TCREATE:
      free(msg->params.tcreate.name);
      break;

    default:
      break;
  }

  rfs__9p_msg_init(msg);
}

uint32_t rfs__9p_msg_size(const rfs__9p_msg_t* msg) {
  assert(msg != NULL);

  size_t size = 0;

  // size of the size + type + tag fields
  size += sizeof(uint32_t);
  size += sizeof(uint8_t);
  size += sizeof(uint16_t);

  switch(msg->type) {
    case RFS__9P_TVERSION:
    case RFS__9P_RVERSION:
      size += sizeof(uint32_t); // msize
      size += str_size(msg->params.version.version);
      break;

    case RFS__9P_TAUTH:
      size += sizeof(uint32_t); // afid
      size += str_size(msg->params.tauth.uname);
      size += str_size(msg->params.tauth.aname);
      break;

    case RFS__9P_RAUTH:
      size += sizeof(rfs_qid_t);
      break;

    case RFS__9P_RERROR:
      size += str_size(msg->params.rerror.ename);
      break;

    case RFS__9P_TFLUSH:
      size += sizeof(uint16_t); // oldtag
      break;

    case RFS__9P_RFLUSH:
      break;

    case RFS__9P_TATTACH:
      size += sizeof(uint32_t); // fid;
      size += sizeof(uint32_t); // afid;
      size += str_size(msg->params.tattach.uname);
      size += str_size(msg->params.tattach.aname);
      break;

    case RFS__9P_RATTACH:
      size += sizeof(rfs_qid_t);
      break;

    case RFS__9P_TWALK:
      size += sizeof(uint32_t); // fid
      size += sizeof(uint32_t); // newfid
      size += sizeof(uint16_t); // nwname

      assert(msg->params.twalk.nwname <= RFS__9P_MAXWELEM);

      for(uint16_t i = 0; i < msg->params.twalk.nwname; ++i) {
        size += str_size(msg->params.twalk.wname[i]);
      }      
      break;

    case RFS__9P_RWALK:
      size += sizeof(uint16_t); // nwqid

      assert(msg->params.rwalk.nwqid <= RFS__9P_MAXWELEM);

      size += (qid_size() * msg->params.rwalk.nwqid);
      break;

    case RFS__9P_TOPEN:
      size += sizeof(uint32_t); // fid
      size += sizeof(uint8_t); // mode
      break;

    case RFS__9P_ROPEN:
      size += sizeof(rfs_qid_t); // qid
      size += sizeof(uint32_t); // iounit
      break;

    case RFS__9P_TCREATE:
      size += sizeof(uint32_t); // fid
      size += str_size(msg->params.tcreate.name);
      size += sizeof(uint32_t); // perm
      size += sizeof(uint8_t); // mode
      break;

    case RFS__9P_RCREATE:
      size += sizeof(rfs_qid_t); // qid
      size += sizeof(uint32_t); // iounit
      break;

    case RFS__9P_TREAD:
      size += sizeof(uint32_t); // fid
      size += sizeof(uint64_t); // offset
      size += sizeof(uint32_t); // count
      break;

    case RFS__9P_RREAD:
      size += sizeof(uint32_t); // count
      size += msg->params.rread.count; // data
      break;

    case RFS__9P_TWRITE:
      size += sizeof(uint32_t); // fid
      size += sizeof(uint64_t); // offset
      size += sizeof(uint32_t); // count
      size += msg->params.twrite.count; // data
      break;

    case RFS__9P_RWRITE:
      size += sizeof(uint32_t); // count
      break;

    case RFS__9P_TCLUNK:
      size += sizeof(uint32_t); // fid
      break;

    case RFS__9P_RCLUNK:
      break; // empty

    case RFS__9P_TREMOVE:
      size += sizeof(uint32_t); // fid
      break;

    case RFS__9P_RREMOVE:
      break; // empty

    case RFS__9P_TSTAT:
      size += sizeof(uint32_t); // fid
      break;

    case RFS__9P_RSTAT:
      size += rfs__9p_stat_size(msg->params.rstat.stat);
      break;

    case RFS__9P_TWSTAT:
      size += sizeof(uint32_t); // fid
      size += rfs__9p_stat_size(msg->params.twstat.stat);
      break;

    case RFS__9P_RWSTAT:
      break; // empty

    default:
      return 0;
  }

  return size;
}

size_t rfs__9p_msg_pack(rfs__9p_msg_t* msg,
                        unsigned char* buf,
                        size_t bufsize) {
  assert(msg != NULL);
  assert(buf != NULL);

  msg->size = rfs__9p_msg_size(msg);

  if(msg->size > bufsize)
    return 0;

  size_t used = 0;
  used += uint32_pack(msg->size, buf + used, bufsize - used);
  used += uint8_pack (msg->type, buf + used, bufsize - used);
  used += uint16_pack(msg->tag, buf + used, bufsize - used);

  switch(msg->type) {
    case RFS__9P_TVERSION:
    case RFS__9P_RVERSION:
      // per man 5 version, tag must be set to NOTAG
      assert(msg->tag == RFS__9P_NOTAG);

      // per man 5 version, the version field must start with 9P
      // unless there is a error in the response, then it must be unknown
      assert(msg->params.version.version != NULL);
      assert(strlen(msg->params.version.version) >= 2);

      if(msg->type == RFS__9P_TVERSION)
        assert(strncmp(msg->params.version.version, "9P", 2) == 0);
      else
        assert(strncmp(msg->params.version.version, "9P", 2) == 0
            || (strlen(msg->params.version.version) == 7
              && strncmp(msg->params.version.version, "unknown", 7) == 0));

      used += uint32_pack(msg->params.version.msize, buf + used, bufsize - used);
      used += str_pack   (msg->params.version.version, buf + used, bufsize - used);
      break;

    case RFS__9P_TAUTH:
      used += uint32_pack(msg->params.tauth.afid, buf + used, bufsize - used);
      used += str_pack   (msg->params.tauth.uname, buf + used, bufsize - used);
      used += str_pack   (msg->params.tauth.aname, buf + used, bufsize - used);
      break;

    case RFS__9P_RAUTH:
      used += qid_pack(&(msg->params.rauth.aqid), buf + used, bufsize - used);
      break;

    case RFS__9P_RERROR:
      used += str_pack(msg->params.rerror.ename, buf + used, bufsize - used);
      break;

    case RFS__9P_TFLUSH:
      used += uint16_pack(msg->params.tflush.oldtag, buf + used, bufsize - used);
      break;

    case RFS__9P_RFLUSH:
      break;

    case RFS__9P_TATTACH:
      used += uint32_pack(msg->params.tattach.fid, buf + used, bufsize - used);
      used += uint32_pack(msg->params.tattach.afid, buf + used, bufsize - used);
      used += str_pack   (msg->params.tattach.uname, buf + used, bufsize - used);
      used += str_pack   (msg->params.tattach.aname, buf + used, bufsize - used);
      break;

    case RFS__9P_RATTACH:
      used += qid_pack(&(msg->params.rattach.qid), buf + used, bufsize - used);
      break;

    case RFS__9P_TWALK:
      used += uint32_pack(msg->params.twalk.fid, buf + used, bufsize - used);
      used += uint32_pack(msg->params.twalk.newfid, buf + used, bufsize - used);
      used += uint16_pack(msg->params.twalk.nwname, buf + used, bufsize - used);

      assert(msg->params.twalk.nwname <= RFS__9P_MAXWELEM);

      for(uint16_t i = 0; i < msg->params.twalk.nwname; ++i) {
        // Can't walk to a null dirent
        assert(msg->params.twalk.wname[i] != NULL);
        // Per man 5 walk, the path '.' is not used in the 9P protocol
        assert(strlen(msg->params.twalk.wname[i]) > 1
            || *(msg->params.twalk.wname[i]) != '.');

        used += str_pack(msg->params.twalk.wname[i], buf + used, bufsize - used);
      }
      break;

    case RFS__9P_RWALK:
      used += uint16_pack(msg->params.rwalk.nwqid, buf + used, bufsize - used);

      assert(msg->params.rwalk.nwqid <= RFS__9P_MAXWELEM);

      for(uint16_t i = 0; i < msg->params.rwalk.nwqid; ++i) {
        used += qid_pack(&(msg->params.rwalk.wqid[i]), buf + used, bufsize - used);
      }
      break;

    case RFS__9P_TOPEN:
      used += uint32_pack(msg->params.topen.fid, buf + used, bufsize - used);
      used += uint8_pack (msg->params.topen.mode, buf + used, bufsize - used);
      break;

    case RFS__9P_ROPEN:
      used += qid_pack   (&(msg->params.ropen.qid), buf + used, bufsize - used);
      used += uint32_pack(msg->params.ropen.iounit, buf + used, bufsize - used);
      break;

    case RFS__9P_TCREATE:
      used += uint32_pack(msg->params.tcreate.fid, buf + used, bufsize - used);
      used += str_pack   (msg->params.tcreate.name, buf + used, bufsize - used);
      used += uint32_pack(msg->params.tcreate.perm, buf + used, bufsize - used);
      used += uint8_pack (msg->params.tcreate.mode, buf + used, bufsize - used);
      break;

    case RFS__9P_RCREATE:
      used += qid_pack   (&(msg->params.rcreate.qid), buf + used, bufsize - used);
      used += uint32_pack(msg->params.rcreate.iounit, buf + used, bufsize - used);
      break;

    case RFS__9P_TREAD:
      used += uint32_pack(msg->params.tread.fid, buf + used, bufsize - used);
      used += uint64_pack(msg->params.tread.offset, buf + used, bufsize - used);
      used += uint32_pack(msg->params.tread.count, buf + used, bufsize - used);
      break;

    case RFS__9P_RREAD:
      used += uint32_pack(msg->params.rread.count, buf + used, bufsize - used);
      memcpy(buf + used, msg->params.rread.data, msg->params.rread.count);
      used += msg->params.rread.count;
      break;

    case RFS__9P_TWRITE:
      used += uint32_pack(msg->params.twrite.fid, buf + used, bufsize - used);
      used += uint64_pack(msg->params.twrite.offset, buf + used, bufsize - used);
      used += uint32_pack(msg->params.twrite.count, buf + used, bufsize - used);
      memcpy(buf + used, msg->params.twrite.data, msg->params.twrite.count);
      used += msg->params.twrite.count;
      break;

    case RFS__9P_RWRITE:
      used += uint32_pack(msg->params.rwrite.count, buf + used, bufsize - used);
      break;

    case RFS__9P_TCLUNK:
      used += uint32_pack(msg->params.tclunk.fid, buf + used, bufsize - used);
      break;

    case RFS__9P_RCLUNK:
      break;

    case RFS__9P_TREMOVE:
      used += uint32_pack(msg->params.tremove.fid, buf + used, bufsize - used);
      break;

    case RFS__9P_RREMOVE:
      break;

    case RFS__9P_TSTAT:
      used += uint32_pack(msg->params.tstat.fid, buf + used, bufsize - used);
      break;

    case RFS__9P_RSTAT:
      used += rfs__9p_stat_pack(msg->params.rstat.stat,
                                buf + used, bufsize - used);
      break;

    case RFS__9P_TWSTAT:
      used += uint32_pack(msg->params.twstat.fid, buf + used, bufsize - used);
      used += rfs__9p_stat_pack(msg->params.twstat.stat,
                                buf + used, bufsize - used);
      break;

    case RFS__9P_RWSTAT:
      break;

    default:
      return 0;
  }

  assert(used == msg->size);
  assert(used <= bufsize);

  return used;
}

size_t rfs__9p_msg_unpack(unsigned char* buf,
                          size_t bufsize,
                          rfs__9p_msg_t* msg) {
  assert(buf != NULL);
  assert(msg != NULL);

  size_t used = uint32_unpack(buf, bufsize, &(msg->size));

  if(msg->size < bufsize)
    return 0;

  used += uint8_unpack (buf + used, bufsize - used, &(msg->type));
  used += uint16_unpack(buf + used, bufsize - used, &(msg->tag));

  switch(msg->type) {
    case RFS__9P_TVERSION:
    case RFS__9P_RVERSION:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.version.msize));
      used += str_unpack   (buf + used, bufsize - used,
                            &(msg->params.version.version));
      break;

    case RFS__9P_TAUTH:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tauth.afid));
      used += str_unpack   (buf + used, bufsize - used,
                            &(msg->params.tauth.uname));
      used += str_unpack   (buf + used, bufsize - used,
                            &(msg->params.tauth.aname));
      break;

    case RFS__9P_RAUTH:
      used += qid_unpack   (buf + used, bufsize - used,
                            &(msg->params.rauth.aqid));
      break;

    case RFS__9P_RERROR:
      used += str_unpack   (buf + used, bufsize - used,
                            &(msg->params.rerror.ename));
      break;

    case RFS__9P_TFLUSH:
      used += uint16_unpack(buf + used, bufsize - used,
                            &(msg->params.tflush.oldtag));
      break;

    case RFS__9P_RFLUSH:
      break; // empty

    case RFS__9P_TATTACH:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tattach.fid));
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tattach.afid));
      used += str_unpack   (buf + used, bufsize - used,
                            &(msg->params.tattach.uname));
      used += str_unpack   (buf + used, bufsize - used,
                            &(msg->params.tattach.aname));
      break;

    case RFS__9P_RATTACH:
      used += qid_unpack   (buf + used, bufsize - used,
                            &(msg->params.rattach.qid));
      break;

    case RFS__9P_TWALK:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.twalk.fid));
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.twalk.newfid));
      used += uint16_unpack(buf + used, bufsize - used,
                            &(msg->params.twalk.nwname));

      // todo: error checking
      assert(msg->params.twalk.nwname <= RFS__9P_MAXWELEM);

      for(uint16_t i = 0; i < msg->params.twalk.nwname; ++i) {
        used += str_unpack (buf + used, bufsize - used,
                            &(msg->params.twalk.wname[i]));
      }
      break;

    case RFS__9P_RWALK:
      used += uint16_unpack(buf + used, bufsize - used,
                            &(msg->params.rwalk.nwqid));

      // todo: error checking
      assert(msg->params.rwalk.nwqid <= RFS__9P_MAXWELEM);

      for(uint16_t i = 0; i < msg->params.rwalk.nwqid; ++i) {
        used += qid_unpack (buf + used, bufsize - used,
                            &(msg->params.rwalk.wqid[i]));
      }
      break;

    case RFS__9P_TOPEN:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.topen.fid));
      used += uint8_unpack (buf + used, bufsize - used,
                            &(msg->params.topen.mode));
      break;

    case RFS__9P_ROPEN:
      used += qid_unpack   (buf + used, bufsize - used,
                            &(msg->params.ropen.qid));
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.ropen.iounit));
      break;

    case RFS__9P_TCREATE:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tcreate.fid));
      used += str_unpack   (buf + used, bufsize - used,
                            &(msg->params.tcreate.name));
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tcreate.perm));
      used += uint8_unpack (buf + used, bufsize - used,
                            &(msg->params.tcreate.mode));
      break;

    case RFS__9P_RCREATE:
      used += qid_unpack   (buf + used, bufsize - used,
                            &(msg->params.rcreate.qid));
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.rcreate.iounit));
      break;

    case RFS__9P_TREAD:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tread.fid));
      used += uint64_unpack(buf + used, bufsize - used,
                            &(msg->params.tread.offset));
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tread.count));
      break;

    case RFS__9P_RREAD:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.rread.count));

      // todo error checking
      msg->params.rread.data = buf + used;
      used += msg->params.rread.count;
      break;

    case RFS__9P_TWRITE:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.twrite.fid));
      used += uint64_unpack(buf + used, bufsize - used,
                            &(msg->params.twrite.offset));
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.twrite.count));

      // todo error checking
      msg->params.twrite.data = buf + used;
      used += msg->params.twrite.count;
      break;

    case RFS__9P_RWRITE:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.rwrite.count));
      break;

    case RFS__9P_TCLUNK:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tclunk.fid));
      break;

    case RFS__9P_RCLUNK:
      break; // empty

    case RFS__9P_TREMOVE:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tremove.fid));
      break;

    case RFS__9P_RREMOVE:
      break; // empty

    case RFS__9P_TSTAT:
      used += uint32_unpack(buf + used, bufsize - used,
                            &(msg->params.tstat.fid));
      break;

    case RFS__9P_RSTAT:
      used += rfs__9p_stat_unpack(buf + used, bufsize - used,
                                  msg->params.rstat.stat);
      break;

    case RFS__9P_TWSTAT:
      used += uint32_unpack      (buf + used, bufsize - used,
                                  &(msg->params.twstat.fid));
      used += rfs__9p_stat_unpack(buf + used, bufsize - used,
                                  msg->params.twstat.stat);
      break;

    case RFS__9P_RWSTAT:
      break; // empty

    default:
      return 0;
  }

  return used;
}
