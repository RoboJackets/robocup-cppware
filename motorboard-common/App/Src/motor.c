#include "motor.h"
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_gpio.h"
#include "tim.h"
#include <stdbool.h>

motor_hall_state_t get_motor_hall_state(void) {
  motor_hall_state_t state = {};

  state.chA = HAL_GPIO_ReadPin(HALL_A_GPIO_Port, HALL_A_Pin);
  state.chB = HAL_GPIO_ReadPin(HALL_B_GPIO_Port, HALL_B_Pin);
  state.chC = HAL_GPIO_ReadPin(HALL_C_GPIO_Port, HALL_C_Pin);

  return state;
}

static int16_t hall_to_angle[8] = {
    -1, // cba (not possible)
    2,  // cbA 120° [2]
    4,  // cBa 240° [4]
    3,  // cBA 180° [3]
    0,  // Cba 000° [0]
    1,  // CbA 060° [1]
    5,  // CBa 300° [5]
    -1  // CBA (not possible)
};

int8_t halls_to_index(motor_hall_state_t state) {

  // The below packs hall channels A,B,C into the lower 3 bits
  // 0b00000CBA
  uint8_t index = 0;
  if (state.chA == true) {
    index |= 0x1;
  }
  if (state.chB == true) {
    index |= 0x2;
  }
  if (state.chC == true) {
    index |= 0x4;
  }

  return hall_to_angle[index];
}

void motor_set_duty_cycle(float duty) {
  uint32_t duty_thresh = duty * 640;

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty_thresh);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty_thresh);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, duty_thresh);
}

static void set_single_motor_phase(enum motor_phase_state_t state,
                                   uint32_t ch) {
  if (state == MOTOR_PHASE_HIGH) { // high side on
    HAL_TIM_PWM_Start(&htim1, ch);
    HAL_TIMEx_PWMN_Stop(&htim1, ch);
  } else if (state == MOTOR_PHASE_LOW) { // low side on
    HAL_TIM_PWM_Stop(&htim1, ch);
    HAL_TIMEx_PWMN_Start(&htim1, ch);
  } else { // floating
    HAL_TIM_PWM_Stop(&htim1, ch);
    HAL_TIMEx_PWMN_Stop(&htim1, ch);
  }
}

void motor_set_phase_state(enum motor_phase_state_t stateU,
                           enum motor_phase_state_t stateV,
                           enum motor_phase_state_t stateW) {
  set_single_motor_phase(stateU, TIM_CHANNEL_1);
  set_single_motor_phase(stateV, TIM_CHANNEL_2);
  set_single_motor_phase(stateW, TIM_CHANNEL_3);
}