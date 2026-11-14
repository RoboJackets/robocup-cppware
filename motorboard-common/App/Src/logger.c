#include "logger.h"
#include "usart.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// circular log buffer for storing log messages
// we're okay with overwriting old log messages
static char log_buf[LOG_BUF_BLOCKS * LOG_BUF_BLOCK_SIZE];
static int16_t log_read_ptr = 0;
static int16_t log_write_ptr = 0;
static int16_t log_buf_count = 0;

// claim a log buffer block for writing
static char *claim_block() {
  char *ret_block = &log_buf[log_write_ptr * LOG_BUF_BLOCK_SIZE];
  log_write_ptr = (log_write_ptr + 1) % LOG_BUF_BLOCKS;
  log_buf_count++;
  if (log_buf_count > LOG_BUF_BLOCKS) {
    log_buf_count = LOG_BUF_BLOCKS;

    // we also need to advance read ptr, since it was
    // overwritten by the new block
    log_read_ptr = (log_read_ptr + 1) % LOG_BUF_BLOCKS;
  }
  return ret_block;
}

static char *read_block() {
  if (log_buf_count == 0) {
    return NULL;
  }
  char *ret_block = &log_buf[log_read_ptr * LOG_BUF_BLOCK_SIZE];
  log_read_ptr = (log_read_ptr + 1) % LOG_BUF_BLOCKS;
  log_buf_count--;
  return ret_block;
}

void log_flush() {
  char *msg = read_block();
  if (msg == NULL) {
    return;
  }
  int16_t size = strnlen(msg, LOG_BUF_BLOCK_SIZE);
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, size, 100);
}

static void log_common(const char *fmt, va_list args) {
  // claim a block
  char *new_block = claim_block();
  vsnprintf(new_block, LOG_BUF_BLOCK_SIZE, fmt, args);

  // force the block to end in a newline
  int16_t size = strnlen(new_block, LOG_BUF_BLOCK_SIZE);
  if (size < LOG_BUF_BLOCK_SIZE - 1) {
    new_block[size] = '\n';
    new_block[size + 1] = '\0';
  } else {
    new_block[LOG_BUF_BLOCK_SIZE - 1] = '\n';
  }
}

// Only logs in debug mode
void log_info(const char *fmt, ...) {
#ifndef NDEBUG
  // assert(__get_IPSR() != 0); // ensure we are not in an interrupt context

  va_list args;
  va_start(args, fmt);
  log_common(fmt, args);
  va_end(args);
#endif /* NDEBUG*/
}

// Always logs
void log_error(const char *fmt, ...) {
  // assert(__get_IPSR() != 0); // ensure we are not in an interrupt context

  va_list args;
  va_start(args, fmt);
  log_common(fmt, args);
  va_end(args);
}
