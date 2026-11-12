#include "app.h"
#include "board_constants.h"
#include "i2c.h"
#include "i2c_generic.h"
#include "logger.h"
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_gpio.h"
#include "temp.h"
#include <stdio.h>

/**
 * THIS IS A PLACEHOLDER FILE WHILE DRIVERS ARE WRITTEN AND TESTED!
 */

void app_start() {
  // initialize I2C peripheral

  enum boardtype board_type = get_board_type();
  if (board_type == BOARD_TYPE_UNKNOWN) {
    log_info("Unknown board type");
  } else if (board_type == BOARD_TYPE_DRIVE_V1) {
    log_info("Drive board v1 identified!");
  } else if (board_type == BOARD_TYPE_DRIBBLER_v1) {
    log_info("Dribbler board identified!");
  }
  flush_logs();

  while (1) {
    temp_readings_t temps = get_temperature_readings();
    log_info("Ambient: %d C", (int)temps.ambient);
    log_info("Die: %d C", (int)temps.die);

    HAL_GPIO_TogglePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
    HAL_Delay(500);
    flush_logs();
    flush_logs();
  }
}