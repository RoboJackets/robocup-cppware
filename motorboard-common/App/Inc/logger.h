#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdarg.h>

// The log buffer is borken up into n blocks of 64 bytes each.
#define LOG_BUF_BLOCKS 8 // use multiples of 2 if possible.
#define LOG_BUF_BLOCK_SIZE 64

void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);

void log_flush();

#endif /* __LOGGER_H */