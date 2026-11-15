#include "app.h"
#include "adc.h"
#include "board_constants.h"
#include "encoder.h"
#include "fix32.h"
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

// velocity loop
static volatile int32_t velocity = 0;
static volatile int32_t velocity_setpoint = 0;

// current loop
static volatile fix32_t current = 0;
static volatile fix32_t current_setpoint = 0;
static volatile fix32_t duty = 0;

static volatile motor_direction_t motor_direction = MOTOR_FWD;
static volatile bool motor_coast = true;

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

  // calibrate the ADC offset
  motor_adc_calibrate(adc_out.current);

  while (1) {
    await_uart();
    log_info("TEST START");
    log_flush();
    sample_idx = 0;
    memset(sample_datastore, 0, sizeof(sample_datastore));
    HAL_TIM_Base_Start_IT(&htim16);
    HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, false);

    /* TEST START */

    velocity_setpoint = 150000;
    motor_coast = false;

    HAL_Delay(300);

    velocity_setpoint = -150000;

    HAL_Delay(500);

    motor_coast = true;
    velocity_setpoint = 0;

    /* TEST END */

    HAL_GPIO_WritePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin, true);
    HAL_TIM_Base_Stop_IT(&htim16);

    log_info("TEST END");
    log_flush();

    HAL_Delay(1000);

    log_info("DATA START");
    log_flush();

    for (int i = 0; i < sample_idx; i++) {
      log_info("%d,%d", i, sample_datastore[i] * 8);
      log_flush();
    }

    log_info("DATA END");
    log_flush();
  }
}

// 1Khz sampling loop
void TIM16_ISR() {
  if (sample_idx < 1024) {
    sample_datastore[sample_idx] = velocity / 8;
    sample_idx++;
  }
}

static volatile float temp_res = 0.0;

// 4kHz velocity control loop
void velocity_loop() {
  static fix32_t velocity_acc = FIX_FROM_FLOAT(0.0);
  static const fix32_t velocity_kp = FIX_FROM_FLOAT(100);
  static const fix32_t velocity_ki = FIX_FROM_FLOAT(0.001);
  static const fix32_t I_MAX = FIX_FROM_FLOAT(4.0);
  static const fix32_t I_MIN = FIX_FROM_FLOAT(-4.0);

  velocity_update();
  velocity = velocity_get();

  if (motor_coast == true) {
    velocity_acc = FIX_FROM_FLOAT(0.0);
    current_setpoint = FIX_FROM_FLOAT(0.0);
    return;
  }

  fix32_t error = fix_sub(velocity_setpoint, velocity);
  velocity_acc = fix_add(velocity_acc, error);

  current_setpoint =
      fix_add(fix_mul(velocity_kp, error), fix_mul(velocity_ki, velocity_acc));
  current_setpoint = fix_clamp(current_setpoint, I_MIN, I_MAX);
}

// 25kHz current loop
void current_loop(fix32_t current_k) {

  static fix32_t current_acc = FIX_FROM_FLOAT(0.0);
  static const fix32_t current_kp = FIX_FROM_FLOAT(0.04125);
  static const fix32_t current_ki = FIX_FROM_FLOAT(0.002);
  static const fix32_t D_MAX = FIX_FROM_FLOAT(0.95);
  static const fix32_t D_MIN = FIX_FROM_FLOAT(-0.95);

  if (motor_coast == true) {
    current_acc = FIX_FROM_FLOAT(0.0);
    motor_set_duty_cycle_fix(FIX_FROM_FLOAT(0.0));
    return;
  }

  fix32_t error = fix_sub(current_setpoint, current_k);
  fix32_t next_acc = fix_add(current_acc, error);

  fix32_t duty_raw =
      fix_add(fix_mul(current_kp, error), fix_mul(current_ki, current_acc));

  if (duty < 0) {
    motor_direction = MOTOR_REV;
  } else {
    motor_direction = MOTOR_FWD;
  }

  duty = fix_clamp(duty_raw, D_MIN, D_MAX);

  // anti-windup for the integrator
  if (duty_raw == duty || (duty_raw >= D_MAX && error < 0) ||
      (duty_raw <= D_MIN && error > 0)) {
    current_acc = next_acc;
  }

  motor_set_duty_cycle_fix(fix_abs(duty));

  // trigger the commutation itr
  __HAL_GPIO_EXTI_GENERATE_SWIT(HALL_A_Pin);
}

// 4kHz velocity control loop
void TIM14_ISR() { velocity_loop(); }

// Called on hall edge update
void HALL_ISR() {

  motor_hall_state_t halls = get_motor_hall_state();
  int8_t index = halls_to_index(halls);

  // are we being commanded to brake?

  // current velocity is in the opposite direction of the commanded direction
  bool motor_braking = false;
  if (velocity < -20000 && motor_direction == MOTOR_FWD) {
    motor_braking = true;
  }
  if (velocity > 20000 && motor_direction == MOTOR_REV) {
    motor_braking = true;
  }

  if (motor_coast) {
    motor_set_phase_state(MOTOR_PHASE_FLOAT, MOTOR_PHASE_FLOAT,
                          MOTOR_PHASE_FLOAT);
  } else if (motor_braking) {
    motor_set_phase_state(MOTOR_PHASE_LOW, MOTOR_PHASE_LOW, MOTOR_PHASE_LOW);
  } else {
    enum motor_phase_state_t *motor_phases = (motor_direction == MOTOR_FWD)
                                                 ? phase_table_fwd[index]
                                                 : phase_table_rev[index];
    motor_set_phase_state(motor_phases[0], motor_phases[1], motor_phases[2]);
  }
}

// Called every time an ADC conversion is complete
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  static bool even_tick = false;
  static uint32_t current_sum = 0;

  if (hadc->Instance == ADC1) {
    current_sum += adc_out.current;
    if (even_tick) {
      // only convert every other tick, averaging out

      current = motor_adc_to_current(current_sum / 2);
      current_sum = 0;
      current_loop(current);
    }
    even_tick = !even_tick;
  }
}