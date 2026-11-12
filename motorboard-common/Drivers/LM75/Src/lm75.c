#include "lm75.h"
#include "i2c_generic.h"
#include <stdint.h>

#define LM75_TEMP_MIN_C (-55.0f)
#define LM75_TEMP_MAX_C (125.0f)

/* Select a register via the pointer byte, then read `len` bytes from it. */
static int lm75_read_reg(const lm75_t *sensor, enum lm75_register reg,
                         uint8_t *data, size_t len) {
  uint8_t ptr = (uint8_t)reg;
  if (sensor->bus->write(sensor->bus->ctx, sensor->addr, &ptr, 1) != 0) {
    return -1;
  }
  return sensor->bus->read(sensor->bus->ctx, sensor->addr, data, len);
}

/* Read a 16-bit temperature-format register (TEMP, OT, HYST) in degrees C.
 * Format: 9-bit two's complement in bits 15..7, 0.5 degC/LSB. */
static float lm75_read_temp_reg(const lm75_t *sensor, enum lm75_register reg) {
  if (sensor == NULL) {
    return 0.0f;
  }

  uint8_t buf[2];
  if (lm75_read_reg(sensor, reg, buf, 2) != 0) {
    return 0.0f;
  }

  int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
  raw >>= 7; /* arithmetic shift preserves sign */
  return (float)raw * 0.5f;
}

static void lm75_write_temp_reg(const lm75_t *sensor, enum lm75_register reg,
                                float temperature) {
  if (sensor == NULL) {
    return;
  }

  if (temperature < LM75_TEMP_MIN_C) {
    temperature = LM75_TEMP_MIN_C;
  } else if (temperature > LM75_TEMP_MAX_C) {
    temperature = LM75_TEMP_MAX_C;
  }

  /* Round to nearest 0.5 degC step */
  float steps = temperature * 2.0f;
  int16_t raw = (int16_t)(steps >= 0.0f ? steps + 0.5f : steps - 0.5f);
  uint16_t reg_val = (uint16_t)((uint16_t)raw << 7);

  uint8_t transmission[3];
  transmission[0] = (uint8_t)reg;
  transmission[1] = (uint8_t)(reg_val >> 8);
  transmission[2] = (uint8_t)(reg_val & 0xFF);
  sensor->bus->write(sensor->bus->ctx, sensor->addr, transmission, 3);
}

void lm75_set_config(const lm75_t *sensor, const lm75_config_t *config) {
  if (sensor == NULL || config == NULL) {
    return;
  }

  uint8_t config_reg = 0;
  config_reg |= (uint8_t)config->os_queue << 3;
  config_reg |= (uint8_t)config->os_polarity << 2;
  config_reg |= (uint8_t)config->os_mode << 1;
  config_reg |= (uint8_t)config->mode;

  uint8_t transmission[2];
  transmission[0] = LM75_REG_CONFIG;
  transmission[1] = config_reg;
  sensor->bus->write(sensor->bus->ctx, sensor->addr, transmission, 2);
}

lm75_config_t lm75_get_config(const lm75_t *sensor) {
  lm75_config_t config = {LM75_OS_QUEUE1, LM75_OS_POL_LOW, LM75_OS_COMPARATOR,
                          LM75_MODE_NORMAL}; /* power-on default */
  if (sensor == NULL) {
    return config;
  }

  uint8_t config_reg;
  if (lm75_read_reg(sensor, LM75_REG_CONFIG, &config_reg, 1) != 0) {
    return config;
  }

  config.os_queue = (enum lm75_config_os_queue)((config_reg >> 3) & 0x03);
  config.os_polarity = (enum lm75_config_os_polarity)((config_reg >> 2) & 0x01);
  config.os_mode = (enum lm75_config_os_mode)((config_reg >> 1) & 0x01);
  config.mode = (enum lm75_config_mode)(config_reg & 0x01);
  return config;
}

float lm75_read_temperature(const lm75_t *sensor) {
  return lm75_read_temp_reg(sensor, LM75_REG_TEMP);
}

void lm75_set_overtemp(const lm75_t *sensor, float temperature) {
  lm75_write_temp_reg(sensor, LM75_REG_OT, temperature);
}

float lm75_get_overtemp(const lm75_t *sensor) {
  return lm75_read_temp_reg(sensor, LM75_REG_OT);
}

void lm75_set_hysteresis(const lm75_t *sensor, float temperature) {
  lm75_write_temp_reg(sensor, LM75_REG_HYST, temperature);
}

float lm75_get_hysteresis(const lm75_t *sensor) {
  return lm75_read_temp_reg(sensor, LM75_REG_HYST);
}