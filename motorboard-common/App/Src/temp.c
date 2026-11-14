#include "temp.h"
#include "adc.h"
#include "board_constants.h"
#include "i2c.h"
#include "i2c_generic.h"
#include "lm75.h"
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_adc.h"
#include <sys/_intsup.h>

/* I2C Devices */

static int i2c1_write(void *ctx, uint8_t addr, const uint8_t *data,
                      size_t len_) {
  I2C_HandleTypeDef *i2c = (I2C_HandleTypeDef *)ctx;
  if (HAL_I2C_Master_Transmit(i2c, addr << 1, (uint8_t *)data, len_,
                              HAL_MAX_DELAY) != HAL_OK) {
    return -1;
  }
  return 0;
}

static int i2c1_read(void *ctx, uint8_t addr, uint8_t *data, size_t len_) {
  I2C_HandleTypeDef *i2c = (I2C_HandleTypeDef *)ctx;
  if (HAL_I2C_Master_Receive(i2c, addr << 1, data, len_, HAL_MAX_DELAY) !=
      HAL_OK) {
    return -1;
  }
  return 0;
}

static const i2c_bus_t i2c1_bus = {
    .ctx = &hi2c1,
    .write = i2c1_write,
    .read = i2c1_read,
};

static lm75_t lm75 = {
    .bus = &i2c1_bus,
    .addr = I2C_ADDR_LM75,
};

static temp_readings_t last_temps = {0.0, 0.0, 0.0};

temp_readings_t temp_get_readings(void) { return last_temps; }

float temp_get_ambient() {
  float ambient = lm75_read_temperature(&lm75);
  last_temps.ambient = ambient;
  return ambient;
}

float temp_get_motor(void) {
  if (get_board_type() == BOARD_TYPE_DRIBBLER_v1) {
    // TODO: Placeholder untl chip is decided
    return 0.0;
  } else {
    return 0.0;
  }
}

// VDDA is 3.3V
static uint32_t VDDA_MV = 3300U;
float temp_get_die(uint32_t raw_adc) {
  int32_t temp_c =
      __HAL_ADC_CALC_TEMPERATURE(VDDA_MV, raw_adc, ADC_RESOLUTION_12B);
  last_temps.die = temp_c;
  return (float)temp_c;
}