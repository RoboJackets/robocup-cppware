#include "app.h"
#include "adc.h"
#include "board_constants.h"
#include "encoder.h"
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
static volatile float current = 0;

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

    velocity_setpoint = 100000;
    motor_set_duty_cycle(1.0);
    HALL_ISR();
    HAL_TIM_Base_Start_IT(&htim16);

    HAL_Delay(1000);

    HAL_TIM_Base_Stop_IT(&htim16);
    velocity_setpoint = 0;
    motor_set_duty_cycle(0.0);
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

// PID parameters
static volatile int32_t last_error = 0;

static const float FF_OFFSET = 0.205;
static const float FF_SLOPE = 4.3e-6;
static const float VEL_DEADBAND = 20000.0;

static const float param_p = 0.00004;
static const float param_i = 0.0;
static const float param_d = 0.0;

// 8kHz velocity control loop
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

  static float integral = 0.0f;
  static float last_velocity = 0.0f;

  velocity_setpoint = clamp(velocity_setpoint, -200000.0f, 200000.0f);

  if (fabsf(velocity_setpoint) < VEL_DEADBAND) {
    motor_set_duty_cycle(0.0f);
    motor_coast = true;
    integral = 0.0f; // reset state
    last_velocity = velocity;
    HALL_ISR();
  } else {
    motor_coast = false;
    current = motor_adc_to_current(motor_adc_sum / 16);

    float error = velocity_setpoint - velocity;
    float d_meas = velocity - last_velocity; // derivative on measurement
    last_velocity = velocity;

    float ff = (fabsf(velocity_setpoint) > VEL_DEADBAND)
                   ? copysignf(FF_OFFSET + FF_SLOPE * fabsf(velocity_setpoint),
                               velocity_setpoint)
                   : 0.0f;
    // FF_OFFSET = 0.205f, FF_SLOPE = 4.3e-6f
    float u = ff + param_p * error + integral - param_d * d_meas;
    float u_sat = clamp(u, -1.0f, 1.0f);

    // anti-windup: only integrate if unsaturated, or if error pulls output back
    if (u == u_sat || (u > 1.0f && error < 0) || (u < -1.0f && error > 0)) {
      integral = clamp(integral + param_i * error, -1.0f, 1.0f);
    }

    // // crude current limit: cut duty if over limit
    // if (current > CURRENT_LIMIT)
    //   u_sat *= CURRENT_LIMIT / current;

    motor_set_duty_cycle(fabsf(u_sat));
    motor_direction = (u_sat >= 0) ? MOTOR_FWD : MOTOR_REV;
    HALL_ISR();
  }
}

// Called on hall edge update
void HALL_ISR() {
  // if (motor_coast) {
  //   motor_set_phase_state(MOTOR_PHASE_FLOAT, MOTOR_PHASE_FLOAT,
  //                         MOTOR_PHASE_FLOAT);
  //   return;
  // }
  static int8_t last_index = -1;
  motor_hall_state_t halls = get_motor_hall_state();
  int8_t index = halls_to_index(halls);

  if (index != last_index) {
    last_index = index;
    enum motor_phase_state_t *motor_phases = (motor_direction == MOTOR_FWD)
                                                 ? phase_table_fwd[index]
                                                 : phase_table_rev[index];
    // current peaks right before a phase switch
    if (index % 2 == 0) {
      HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_out, 2);
    }
    motor_set_phase_state(motor_phases[0], motor_phases[1], motor_phases[2]);
  }
}