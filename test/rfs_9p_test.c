#include "src/rfs_9p_wire.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_qid(rfs_qid_t* qid) {
  printf("Path: %" PRIu64 ", Version: %" PRIu32 ", Type: %" PRIu8 "\n",
         qid->path, qid->vers, qid->type);
}

static void print_stat(rfs__9p_stat_t* stat) {
  printf("----- File info -----\n");
  printf("Name: '%s'\n", stat->name);
  printf("Owner: '%s'/'%s'\n", stat->uid, stat->gid);
  printf("Type: %" PRIu16 ", Device: %" PRIu32 ", Mode: %" PRIu32 "\n",
         stat->type, stat->dev, stat->mode);
  printf("Size: %" PRIu64 " bytes\n", stat->length);
  print_qid(&(stat->qid));

  time_t mtime = (time_t) stat->mtime;
  time_t atime = (time_t) stat->atime;
  printf("Last modified by '%s' on %s", stat->muid, ctime(&mtime));
  printf("File created at %s", ctime(&atime));
  printf("-----\n\n");
}

static void print_msg(rfs__9p_msg_t* msg) {
  printf("----- Message info -----\n");
  printf("Size: %" PRIu32 ", type %" PRIu8 ", tag %" PRIu16 "\n",
         msg->size, msg->type, msg->tag);

  switch(msg->type) {
    case RFS__9P_TVERSION:
    case RFS__9P_RVERSION:
      printf("Max size: %" PRIu32 ", Version: '%s'\n",
             msg->params.version.msize, msg->params.version.version);
      break;

    case RFS__9P_TWALK:
      printf("fid: %" PRIu32 ", newfid: %" PRIu32 ", num elems: %" PRIu32 "\n",
             msg->params.twalk.fid, msg->params.twalk.newfid,
             msg->params.twalk.nwname);

      printf("Path: ");
      for(uint16_t i = 0; i < msg->params.twalk.nwname; ++i) {
        printf("%s", msg->params.twalk.wname[i]);

        if((i - 1) != msg->params.twalk.nwname)
          printf("/");
      }
      printf("\n");
      break;

    case RFS__9P_RWALK:
      printf("num qids: %" PRIu32 "\n", msg->params.rwalk.nwqid);

      printf("QIDs: ");
      for(uint16_t i = 0; i < msg->params.rwalk.nwqid; ++i) {
        print_qid(&(msg->params.rwalk.wqid[i]));
      }
      printf("\n");
      break;

    case RFS__9P_TWSTAT:
      printf("fid: %" PRIu32 "\n", msg->params.twstat.fid);
      print_stat(msg->params.twstat.stat);
      break;

    default:
      printf("Parameters not displayed\n");
      break;
  }

  printf("-----\n\n");
}

static void test_stat(void) {
  printf("----- Testing stat packing and unpacking -----\n\n");

  rfs__9p_stat_t stat;
  rfs__9p_stat_init(&stat);

  stat.size = 6120;
  stat.type = 312;
  stat.dev = 100143;
  rfs_qid_t q = { .path = 432143214321, .vers = 101, .type = 100 };
  stat.qid = q;
  stat.mode = 654321;
  stat.atime = stat.mtime = time(NULL);
  stat.length = 123123412345;
  stat.name = strdup("test_file");
  stat.uid = strdup("user@localhost");
  stat.gid = strdup("group@localhost");
  stat.muid = strdup("moduser@localhost");

  print_stat(&stat);

  size_t slen = rfs__9p_stat_size(&stat);
  printf("Serialized stat structure will require %zu bytes\n", slen);

  unsigned char* buf = malloc(slen);
  assert(buf != NULL);

  size_t pack = rfs__9p_stat_pack(&stat, buf, slen);
  printf("After serializing, there were %zu bytes used\n", pack);

  rfs__9p_stat_reset(&stat);

  rfs__9p_stat_t ret;
  rfs__9p_stat_init(&ret);
  size_t unpack = rfs__9p_stat_unpack(buf, slen, &ret);

  printf("After deserializing, there were %zu bytes parsed\n\n", unpack);

  print_stat(&ret);
  free(buf);
}

static void test_msg_version(void) {
  printf("----- Testing message version packing and unpacking -----\n\n");

  rfs__9p_msg_t msg;
  rfs__9p_msg_init(&msg);

  msg.type = RFS__9P_TVERSION;
  msg.tag = RFS__9P_NOTAG;
  msg.params.version.msize = UINT8_MAX;
  msg.params.version.version = strdup("9P2000");

  print_msg(&msg);

  size_t slen = rfs__9p_msg_size(&msg);
  printf("Serialized message structure will require %zu bytes\n", slen);

  unsigned char* buf = malloc(slen);
  assert(buf != NULL);

  size_t pack = rfs__9p_msg_pack(&msg, buf, slen);
  printf("After serializing, there were %zu bytes used\n", pack);

  rfs__9p_msg_reset(&msg);

  rfs__9p_msg_t ret;
  rfs__9p_msg_init(&ret);

  size_t unpack = rfs__9p_msg_unpack(buf, slen, &ret);

  printf("After deserializing, there were %zu bytes parsed\n\n", unpack);

  print_msg(&ret);
  free(buf);
}

static void test_msg_twalk(void) {
  printf("----- Testing message Twalk packing and unpacking -----\n\n");

  rfs__9p_msg_t msg;
  rfs__9p_msg_init(&msg);

  msg.type = RFS__9P_TWALK;
  msg.tag = RFS__9P_NOTAG;

  msg.params.twalk.fid = 15243;
  msg.params.twalk.newfid = 15243;
  msg.params.twalk.nwname = 6;
  msg.params.twalk.wname[0] = strdup("");
  msg.params.twalk.wname[1] = strdup("home");
  msg.params.twalk.wname[2] = strdup("robert");
  msg.params.twalk.wname[3] = strdup("Documents");
  msg.params.twalk.wname[4] = strdup("repos");
  msg.params.twalk.wname[5] = strdup("rfs");

  print_msg(&msg);

  size_t slen = rfs__9p_msg_size(&msg);
  printf("Serialized message structure will require %zu bytes\n", slen);

  unsigned char* buf = malloc(slen);
  assert(buf != NULL);

  size_t pack = rfs__9p_msg_pack(&msg, buf, slen);
  printf("After serializing, there were %zu bytes used\n", pack);

  rfs__9p_msg_reset(&msg);

  rfs__9p_msg_t ret;
  rfs__9p_msg_init(&ret);
  size_t unpack = rfs__9p_msg_unpack(buf, slen, &ret);

  printf("After deserializing, there were %zu bytes parsed\n\n", unpack);

  print_msg(&ret);
  free(buf);
}

static void test_msg_rwalk(void) {
  printf("----- Testing message Rwalk packing and unpacking -----\n\n");

  rfs__9p_msg_t msg;
  rfs__9p_msg_init(&msg);

  rfs_qid_t fir = { .path = 123456, .vers = 10, .type = 9 };
  rfs_qid_t sec = { .path = 987654, .vers = 150, .type = 149 };
  rfs_qid_t trd = { .path = 374651, .vers = 631, .type = 254 };

  msg.type = RFS__9P_RWALK;
  msg.tag = RFS__9P_NOTAG;
  msg.params.rwalk.nwqid = 3;
  msg.params.rwalk.wqid[0] = fir;
  msg.params.rwalk.wqid[1] = sec;
  msg.params.rwalk.wqid[2] = trd;

  print_msg(&msg);

  size_t slen = rfs__9p_msg_size(&msg);
  printf("Serialized message structure will require %zu bytes\n", slen);

  unsigned char* buf = malloc(slen);
  assert(buf != NULL);

  size_t pack = rfs__9p_msg_pack(&msg, buf, slen);
  printf("After serializing, there were %zu bytes used\n", pack);

  rfs__9p_msg_reset(&msg);

  rfs__9p_msg_t ret;
  rfs__9p_msg_init(&ret);
  size_t unpack = rfs__9p_msg_unpack(buf, slen, &ret);

  printf("After deserializing, there were %zu bytes parsed\n\n", unpack);

  print_msg(&ret);
  free(buf);
}

int main(void) {
  test_stat();
  test_msg_version();
  test_msg_twalk();
  test_msg_rwalk();

  return EXIT_SUCCESS;
}
