#include "app.h"
#include "adc.h"
#include "board_constants.h"
#include "encoder.h"
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

// CCW from shaft side (facing shell)
enum motor_phase_state_t phase_table_fwd[6][3] = {
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH}, // 0:   C+ B-
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT}, // 60:  A+ B-
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW}, // 120: A+ C-
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW}, // 180: B+ C-
    {MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT}, // 240: B+ A-
    {MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH}  // 300: C+ A-
};
// CW from shaft side
enum motor_phase_state_t phase_table_rev[6][3] = {
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW}, // 0:   B+ C-
    {MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT}, // 60:  B+ A-
    {MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT, MOTOR_PHASE_HIGH}, // 120: C+ A-
    {MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW, MOTOR_PHASE_HIGH}, // 180: C+ B-
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_LOW, MOTOR_PHASE_FLOAT}, // 240: A+ B-
    {MOTOR_PHASE_HIGH, MOTOR_PHASE_FLOAT, MOTOR_PHASE_LOW}  // 300: A+ C-
};

static void await_uart() {
  uint8_t tmp;
  HAL_UART_Receive(&hlpuart1, &tmp, 1, 100000);
}

volatile uint32_t current_adc;

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

  // TIM2 is the encoder
  HAL_TIM_Base_Start_IT(&htim2);

  // Configure current sense ADC
  ADC_ChannelConfTypeDef cfg = {0};
  cfg.Channel = ADC_CHANNEL_9;
  cfg.Rank = ADC_REGULAR_RANK_1;
  cfg.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  HAL_ADC_ConfigChannel(&hadc1, &cfg);

  while (1) {
    // Wait before beginning test cycle
    log_info("Awaiting key press...");
    flush_logs();
    await_uart();
    log_info("TEST START");
    flush_logs();

    int32_t start = HAL_GetTick();

    motor_set_duty_cycle(0.3);
    HALL_ISR(); // Call this once to kickstart the process
    while (1) {
      int32_t miliAmps = motor_adc_to_current(current_adc) * 1000;

      log_info("T: %d, mA:%d", velocity_get(), miliAmps);
      flush_logs();

      if (HAL_GetTick() - start > 2000) {
        break;
      }
    }
    motor_set_duty_cycle(0.8);

    HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, true);

    motor_set_duty_cycle(0);

    log_info("TEST END");
    flush_logs();
  }
}

// 10kHz motion control loop
void TIM14_ISR() { velocity_update(); }

// Called on hall edge update
void HALL_ISR() {
  static int8_t last_index = -1;
  motor_hall_state_t halls = get_motor_hall_state();
  int8_t index = halls_to_index(halls);

  if (index != last_index) {
    last_index = index;
    enum motor_phase_state_t *motor_phases = phase_table_fwd[index];
    motor_set_phase_state(motor_phases[0], motor_phases[1], motor_phases[2]);
    HAL_GPIO_WritePin(SCOPE_OUT_GPIO_Port, SCOPE_OUT_Pin, index % 2);
    if (index % 2 == 1) {
      HAL_ADC_Start_DMA(&hadc1, &current_adc, 1);
    }
  }
}