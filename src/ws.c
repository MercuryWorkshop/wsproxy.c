#include "wsproxy.h"
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define WS_FIN 128
#define WS_TEXT 0x1
#define WS_BINARY 0x2

char *ws_send(char *payload, uint16_t pl_len) {

  size_t frame_len = 0;
  unsigned char frame[10]; // sizeof header + max possible length
  memset(frame, 0, sizeof frame);
  frame[0] = (WS_FIN | WS_BINARY);
  frame_len += 2;

  if (pl_len < 126)
    frame[1] = pl_len;
  else if (pl_len < UINT16_MAX) {
    frame[1] = 126;
    frame_len += 2;
    *(uint16_t *)(&frame[2]) = htobe16(pl_len);
  } else {
    frame[1] = 127;
    frame_len += 8;
    *(uint64_t *)(&frame[2]) = htobe64(pl_len);
  }

  write(fd, frame, frame_len);
  write(fd, payload, pl_len);

  return NULL;
}


int ws_recv() {
  unsigned char frame[2];
  memset(frame, 0, 2);
  if (read(fd, frame, 2) == 0)
    return 1;

  unsigned char fin = frame[0] & 128;
  unsigned char opcode = frame[0] & 15;

  unsigned long length = frame[1] & 127;
  unsigned char masked = frame[1] & 128;
  unsigned char mask[4];
  memset(mask, 0, 4);

  if (length == 126) {
    unsigned short len = 0;
    read(fd, &len, sizeof len);
    length = be16toh(len);
  }

  if (length == 127) {
    unsigned long len = 0;
    read(fd, &len, sizeof len);
    length = be64toh(len);
  }

  if (masked == 128) {
    read(fd, mask, sizeof mask);
  }

  char payload[1310000];
  memset(payload, 0, sizeof payload);
  read(fd, payload, length);

  // goofy ahh protocol
  if (masked == 128) {
    for (unsigned long i = 0; i < length; i++) {
      payload[i] ^= mask[i % 4];
    }
  }

  write(tcpfd, payload, length);

  // write(1,payload, length);
  // write(1,";\n",2);
  printf("length: %lu, fin:%i, opcode:%i, masked: %i\n", length, fin, opcode,
         masked);


  return 0;
}
