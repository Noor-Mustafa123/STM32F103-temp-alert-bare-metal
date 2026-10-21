#include "log_buffer.h"
#include <string.h>

__attribute__((section(".logbuf"))) log_buffer_t g_log;

void log_init(void)
{
    memset(&g_log, 0, sizeof(g_log));
    g_log.magic = LOG_MAGIC;
}

void log_write(const char *msg)
{
    while (*msg)
    {
        g_log.data[g_log.write_pos] = *msg++;
        g_log.write_pos++;
        if (g_log.write_pos >= LOG_DATA_SIZE)
        {
            g_log.write_pos = 0;
            g_log.wrapped = 1;
        }
    }
}
