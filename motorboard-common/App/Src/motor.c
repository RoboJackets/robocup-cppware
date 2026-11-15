#include "motor.h"
#include "adc.h"
#include "board_constants.h"
#include "fix32.h"
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_gpio.h"
#include "tim.h"
#include <stdbool.h>

motor_hall_state_t get_motor_hall_state(void) {
  motor_hall_state_t state = {};
  state.chA = HALL_A_GPIO_Port->IDR & HALL_A_Pin;
  state.chB = HALL_B_GPIO_Port->IDR & HALL_B_Pin;
  state.chC = HALL_C_GPIO_Port->IDR & HALL_C_Pin;
  return state;
}

static const int16_t hall_to_angle[8] = {
    -1, // cba (not possible)
    2,  // cbA 120° [2]
    4,  // cBa 240° [4]
    3,  // cBA 180° [3]
    0,  // Cba 000° [0]
    1,  // CbA 060° [1]
    5,  // CBa 300° [5]
    -1  // CBA (not possible)
};

int8_t halls_to_index(motor_hall_state_t s) {
  return hall_to_angle[(s.chA) | (s.chB << 1) | (s.chC << 2)];
}

void motor_set_duty_cycle(float duty) {
  uint32_t duty_thresh = duty * 640;

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty_thresh);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty_thresh);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, duty_thresh);
}

void motor_set_duty_cycle_fix(fix32_t duty) {
  uint32_t duty_thresh = fix_to_int(fix_mul(duty, FIX_FROM_INT(640)));

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty_thresh);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty_thresh);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, duty_thresh);
}

#define PHASE_MASK                                                             \
  (TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE |           \
   TIM_CCER_CC3E | TIM_CCER_CC3NE)

void motor_set_phase_state(enum motor_phase_state_t u,
                           enum motor_phase_state_t v,
                           enum motor_phase_state_t w) {
  TIM1->CCER = (TIM1->CCER & ~PHASE_MASK) | u | (v << 4) | (w << 8);
}

const fix32_t MOTOR_ADC_TO_CURRENT_SCALE = FIX_FROM_FLOAT(3.3 / 4096.0);
static int32_t MOTOR_ADC_OFFSET = 2048;

void motor_adc_calibrate(int32_t offset) { MOTOR_ADC_OFFSET = offset; }

fix32_t motor_adc_to_current(int32_t adc) {
  static fix32_t multiplier = FIX_ONE;
  static bool precompute = false;

  if (!precompute) {
    const analog_constants_t analog_constants = get_analog_constants();
    multiplier =
        fix_mul(MOTOR_ADC_TO_CURRENT_SCALE, analog_constants.sense_gain);
    precompute = true;
  }

  return fix_mul(FIX_FROM_INT(adc - MOTOR_ADC_OFFSET), multiplier);
}