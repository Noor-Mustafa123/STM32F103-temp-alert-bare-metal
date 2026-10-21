#ifndef LOG_BUFFER_H
#define LOG_BUFFER_H

#include <stdint.h>

#define LOG_MAGIC     0xDEADBEEFU
#define LOG_DATA_SIZE (4096 - 12)

typedef struct {
    uint32_t magic;
    uint16_t write_pos;
    uint8_t  wrapped;
    uint8_t  _pad;
    char     data[LOG_DATA_SIZE];
} log_buffer_t;

void log_init(void);
void log_write(const char *msg);

#endif
