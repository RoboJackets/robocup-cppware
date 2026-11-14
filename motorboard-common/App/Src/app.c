#include "app.h"
#include "adc.h"
#include "board_constants.h"
#include "i2c.h"
#include "i2c_generic.h"
#include "logger.h"
#include "main.h"
#include "motor.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_adc.h"
#include "stm32g0xx_hal_gpio.h"
#include "stm32g0xx_hal_uart.h"
#include "temp.h"
#include "usart.h"
#include <stdio.h>

/**
 * THIS IS A PLACEHOLDER FILE WHILE DRIVERS ARE WRITTEN AND TESTED!
 */

enum motor_phase_state_t phase_table[7][3] = {
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT}, // 0
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH}, // 60
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW}, // 120
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW}, // 180
    {MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH}, // 240
    {MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT}, // 300
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_HIGH, MOTOR_PHASE_HIGH}  // disabled
};

static void await_uart() {
  uint8_t tmp;
  HAL_UART_Receive(&hlpuart1, &tmp, 1, 100000);
}
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
  HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, true);

  // Wait for UART signal

  while (1) {

    // Wait before beginning test cycle
    log_info("Awaiting key press...");
    flush_logs();
    await_uart();
    log_info("TEST START");
    flush_logs();

    motor_set_duty_cycle(0.8);

    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel = ADC_CHANNEL_9;
    cfg.Rank = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLINGTIME_COMMON_2;
    HAL_ADC_ConfigChannel(&hadc1, &cfg);

    volatile uint16_t adc_buf[16];
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 16);
    __HAL_DMA_DISABLE_IT(hadc1.DMA_Handle, DMA_IT_HT | DMA_IT_TC);

    int8_t last_index = -1;

    HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, false);

    int32_t start = HAL_GetTick();

    while (1) {
      motor_hall_state_t halls = get_motor_hall_state();
      int8_t index = halls_to_index(halls);

      if (index != last_index) {
        last_index = index;
        enum motor_phase_state_t *motor_phases = phase_table[index];
        motor_set_phase_state(motor_phases[0], motor_phases[1],
                              motor_phases[2]);
      }

      HAL_GPIO_TogglePin(SCOPE_OUT_GPIO_Port, SCOPE_OUT_Pin);

      if (HAL_GetTick() - start > 60000) {
        break;
      }
    }
    HAL_ADC_Stop_DMA(&hadc1);

    HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, true);

    motor_set_phase_state(MOTOR_PHASE_FLOAT, MOTOR_PHASE_FLOAT,
                          MOTOR_PHASE_FLOAT);

    log_info("TEST END");
    flush_logs();
  }
}