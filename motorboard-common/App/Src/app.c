#include "app.h"
#include "gpio.h"
#include "logger.h"
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_gpio.h"
#include <stdio.h>

void app_start() {
  char *message = "Hello World!\n";

  while (1) {
    HAL_GPIO_TogglePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
    HAL_Delay(1000);
    send_log(message);
  }
}