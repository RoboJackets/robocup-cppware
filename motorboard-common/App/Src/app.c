#include "app.h"
#include "board_constants.h"
#include "i2c.h"
#include "i2c_generic.h"
#include "lm75.h"
#include "logger.h"
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_gpio.h"
#include <stdio.h>

/**
 * THIS IS A PLACEHOLDER FILE WHILE DRIVERS ARE WRITTEN AND TESTED!
 */

int i2c1_write(void *ctx, uint8_t addr, const uint8_t *data, size_t len_) {
  I2C_HandleTypeDef *i2c = (I2C_HandleTypeDef *)ctx;
  if (HAL_I2C_Master_Transmit(i2c, addr, (uint8_t *)data, len_,
                              HAL_MAX_DELAY) != HAL_OK) {
    return -1;
  }
  return 0;
}

int i2c1_read(void *ctx, uint8_t addr, uint8_t *data, size_t len_) {
  I2C_HandleTypeDef *i2c = (I2C_HandleTypeDef *)ctx;
  if (HAL_I2C_Master_Receive(i2c, addr, data, len_, HAL_MAX_DELAY) != HAL_OK) {
    return -1;
  }
  return 0;
}

void app_start() {
  char printBuffer[256];

  // initialize I2C peripheral
  const i2c_bus_t i2c1_bus = {
      .ctx = &hi2c1,
      .write = i2c1_write,
      .read = i2c1_read,
  };

  lm75_t lm75 = {
      .bus = &i2c1_bus,
      .addr = 0x48,
  };

  enum boardtype board_type = get_board_type();
  if (board_type == BOARD_TYPE_UNKNOWN) {
    snprintf(printBuffer, sizeof(printBuffer), "Unknown board type\n");
    send_log(printBuffer);
  } else if (board_type == BOARD_TYPE_DRIVE_V1) {
    snprintf(printBuffer, sizeof(printBuffer), "Drive board v1 identified!\n");
    send_log(printBuffer);
  } else if (board_type == BOARD_TYPE_DRIBBLER_v1) {
    snprintf(printBuffer, sizeof(printBuffer), "Dribbler board identified!\n");
    send_log(printBuffer);
  }

  while (1) {
    float temp = lm75_read_temperature(&lm75);
    snprintf(printBuffer, sizeof(printBuffer), "Temperature: %.2f C\n", temp);
    HAL_GPIO_TogglePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
    HAL_Delay(100);
    send_log(printBuffer);
  }
}