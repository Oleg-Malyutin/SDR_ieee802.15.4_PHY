#ifndef ZEP_H
#define ZEP_H

#include <stdint.h>
#if !(defined(WIN32) || defined(WIN64))
#include <sys/time.h>
#include <sys/param.h>
#else
#include <winsock2.h>
#define htobe32(x) htonl(x)
#endif

#define ZEP_BUFFER_SIZE 256
#define ZEP_PREAMBLE "EX"
#define ZEP_V2_TYPE_DATA 1
#define ZEP_V2_TYPE_ACK 2
#define ZEP_PORT 17754

const unsigned long long EPOCH = 2208988800ULL;

struct __attribute__((__packed__)) zep_header {
    char preamble[2];
    uint8_t version;
};
struct __attribute__((__packed__)) zep_header_ack {
  zep_header header;
  uint8_t type;
  uint32_t sequence = 0;
};
struct __attribute__((__packed__)) zep_header_data {
  zep_header header;
  uint8_t type;
  uint8_t channel_id;
  uint16_t device_id;
  uint8_t lqi_mode;
  uint8_t lqi;
  uint32_t timestamp_s;
  uint32_t timestamp_ns;
  uint32_t sequence = 0;
  uint8_t reserved[10];
  uint8_t length;
};

uint8_t rx_zep_buffer[ZEP_BUFFER_SIZE];

#endif // ZEP_H
