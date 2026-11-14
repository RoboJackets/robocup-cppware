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
#include "tim.h"
#include "usart.h"
#include <stdio.h>

/**
 * THIS IS A PLACEHOLDER FILE WHILE DRIVERS ARE WRITTEN AND TESTED!
 */

enum motor_phase_state_t phase_table[6][3] = {
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT}, // 0
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH}, // 60
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW}, // 120
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW}, // 180
    {MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH}, // 240
    {MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT}  // 300
};

enum motor_phase_state_t phase_table_rev[7][3] = {
    {MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT}, // 0
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW}, // 60
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH}, // 120
    {MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH}, // 180
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW}, // 240
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT}  // 300
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
  HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, false);

  // TIM14 is used for the motion control loop
  HAL_TIM_Base_Start_IT(&htim14);

  while (1) {
    // Wait before beginning test cycle
    log_info("Awaiting key press...");
    flush_logs();
    await_uart();
    log_info("TEST START");
    flush_logs();

    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel = ADC_CHANNEL_9;
    cfg.Rank = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLINGTIME_COMMON_2;
    HAL_ADC_ConfigChannel(&hadc1, &cfg);

    volatile uint16_t adc_buf[16];
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 16);
    __HAL_DMA_DISABLE_IT(hadc1.DMA_Handle, DMA_IT_HT | DMA_IT_TC);

    int32_t start = HAL_GetTick();

    motor_set_duty_cycle(0.8);
    HALL_ISR(); // Call this once to kickstart the process
    while (1) {
      log_info("TEST");
      flush_logs();

      if (HAL_GetTick() - start > 1000) {
        break;
      }
    }
    motor_set_duty_cycle(0.8);
    HAL_ADC_Stop_DMA(&hadc1);

    HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, true);

    motor_set_duty_cycle(0);

    log_info("TEST END");
    flush_logs();
  }
}

// 10kHz motion control loop
void TIM14_ISR() {
  // run every time
}

// Called on hall edge update
void HALL_ISR() {
  static int8_t last_index = -1;
  motor_hall_state_t halls = get_motor_hall_state();
  int8_t index = halls_to_index(halls);

  if (index != last_index) {
    last_index = index;
    enum motor_phase_state_t *motor_phases = phase_table_rev[index];
    motor_set_phase_state(motor_phases[0], motor_phases[1], motor_phases[2]);
  }

  HAL_GPIO_TogglePin(SCOPE_OUT_GPIO_Port, SCOPE_OUT_Pin);
}