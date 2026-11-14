#include "app.h"
#include "adc.h"
#include "board_constants.h"
#include "encoder.h"
#include "fixed.h"
#include "i2c.h"
#include "i2c_generic.h"
#include "logger.h"
#include "main.h"
#include "motor.h"
#include "spi.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_adc.h"
#include "stm32g0xx_hal_gpio.h"
#include "stm32g0xx_hal_spi.h"
#include "stm32g0xx_hal_tim.h"
#include "stm32g0xx_hal_uart.h"
#include "temp.h"
#include "tim.h"
#include "usart.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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

float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

static void await_uart() {
  uint8_t tmp;
  HAL_UART_Receive(&hlpuart1, &tmp, 1, 100000);
}

// ADC
typedef struct {
  uint32_t current;
  uint32_t dietemp;
} adc_out_t;

static volatile adc_out_t adc_out = {0, 0};

static volatile int32_t velocity = 0;
static volatile fix32_t current_fixed = 0;

static volatile int32_t velocity_setpoint = 0;
static volatile motor_direction_t motor_direction = MOTOR_FWD;
static volatile bool motor_coast = false;

static int16_t sample_datastore[1024];
static int16_t sample_idx = 0;

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
  log_flush();
  HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, true);

  // start the ADC DMA
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_out, 2);

  // start TIM1 CH4
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

  // TIM14 is used for the motion control loop
  HAL_TIM_Base_Start_IT(&htim14);

  // TIM2 is the encoder
  HAL_TIM_Base_Start_IT(&htim2);

  while (1) {
    await_uart();
    log_info("TEST START");
    log_flush();
    sample_idx = 0;
    memset(sample_datastore, 0, sizeof(sample_datastore));
    motor_coast = false;
    motor_set_duty_cycle(0.9);
    HALL_ISR();
    HAL_TIM_Base_Start_IT(&htim16);

    HAL_Delay(1000);

    HAL_TIM_Base_Stop_IT(&htim16);
    motor_set_duty_cycle(0.0);
    motor_coast = true;
    HALL_ISR();

    log_info("TEST END");
    log_flush();

    log_info("DATA START");
    log_flush();

    for (int i = 0; i < 1024; i++) {
      log_info("%d,%d", i, sample_datastore[i]);
      log_flush();
    }

    log_info("DATA END");
    log_flush();
  }
}

// 2Khz sampling loop
void TIM16_ISR() {
  HAL_GPIO_TogglePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
  if (sample_idx < 1024) {
    sample_datastore[sample_idx] = velocity / 100;
    sample_idx++;
  }
}

static volatile float temp_res = 0.0;

// 4kHz velocity control loop
void TIM14_ISR() {
  velocity_update();
  velocity = velocity_get();

  static uint32_t motor_adc_window[16] = {0};
  static int16_t motor_adc_idx = 0;
  static uint32_t motor_adc_sum = 0;

  // read the last current sense value
  uint32_t motor_adc = adc_out.current;

  // remove the existing entry
  motor_adc_sum -= motor_adc_window[motor_adc_idx];

  // add the new entry
  motor_adc_sum += motor_adc;
  motor_adc_window[motor_adc_idx] = motor_adc;

  // advance the ptr
  motor_adc_idx = (motor_adc_idx + 1) % 16;
}

// Called on hall edge update
void HALL_ISR() {

  static int8_t last_index = -1;
  motor_hall_state_t halls = get_motor_hall_state();
  int8_t index = halls_to_index(halls);

  if (index != last_index) {
    last_index = index;
  }

  if (motor_coast) {
    motor_set_phase_state(MOTOR_PHASE_FLOAT, MOTOR_PHASE_FLOAT,
                          MOTOR_PHASE_FLOAT);
    motor_set_duty_cycle(0.0);
  } else {
    enum motor_phase_state_t *motor_phases = (motor_direction == MOTOR_FWD)
                                                 ? phase_table_fwd[index]
                                                 : phase_table_rev[index];
    motor_set_phase_state(motor_phases[0], motor_phases[1], motor_phases[2]);
  }
}

// Called every time an ADC conversion is complete
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    if (motor_coast == false) {
      current_fixed = motor_adc_to_current(adc_out.current);
    }
  }
}