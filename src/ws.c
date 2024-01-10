#include "wsproxy.h"
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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


#include <immintrin.h>
int ws_recv() {
  unsigned char frame[2];
  memset(frame, 0, 2);
  if (read(fd, frame, 2) == 0)
    return 1;

  unsigned char fin = frame[0] & 128;
  unsigned char opcode = frame[0] & 15;

  unsigned long length = frame[1] & 127;
  unsigned char masked = frame[1] & 128;

  uint32_t mask = 0;

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
    read(fd, &mask, sizeof mask);
  }

  char *payload = malloc(length*2);
  memset(payload, 0, length*2);
  read(fd, payload, length);



  // goofy ahh protocol
  if (masked == 128) {
    for (unsigned long i = 0; i < length; i+=4) {
      *(uint32_t*)&payload[i] ^= mask;
    }

    
    // __m256i mask_vec = _mm256_set1_epi32(mask);
    // for (size_t i = 0; i < length; i += 32) {
    //     // Load 32 bytes of payload data into a 256-bit vector, xor it, and store it back 
    //     __m256i data_vec = _mm256_loadu_si256((__m256i *)(payload + i));
    //     __m256i decoded_vec = _mm256_xor_si256(data_vec, mask_vec);
    //     _mm256_storeu_si256((__m256i *)(payload + i), decoded_vec);
    // }
  }

  write(tcpfd, payload, length);

  // write(1,payload, length);
  // write(1,";\n",2);
  printf("length: %lu, fin:%i, opcode:%i, masked: %i\n", length, fin, opcode,
         masked);


  return 0;
}
