#ifndef __I2C_GENERIC_H
#define __I2C_GENERIC_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  int (*write)(void *ctx, uint8_t addr, const uint8_t *data, size_t len);
  int (*read)(void *ctx, uint8_t addr, uint8_t *data, size_t len);
  void *ctx; // HAL handle, passed through
} i2c_bus_t;

#endif /* __I2C_GENERIC_H */