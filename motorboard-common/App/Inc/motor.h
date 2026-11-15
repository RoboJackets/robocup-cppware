#ifndef __MOTOR_H
#define __MOTOR_H

#include "fix32.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_tim.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct motor_hall_state_t {
  bool chA;
  bool chB;
  bool chC;
} motor_hall_state_t;

motor_hall_state_t get_motor_hall_state(void);

int8_t halls_to_index(motor_hall_state_t state);

void motor_set_duty_cycle(float duty);
void motor_set_duty_cycle_fix(fix32_t duty);

enum motor_phase_state_t {
  MOTOR_PHASE_HIGH = TIM_CCER_CC1E,
  MOTOR_PHASE_LOW = TIM_CCER_CC1NE,
  MOTOR_PHASE_FLOAT = 0,
};

void motor_set_phase_state(enum motor_phase_state_t stateU,
                           enum motor_phase_state_t stateV,
                           enum motor_phase_state_t stateW);

void motor_adc_calibrate(int32_t offset);
fix32_t motor_adc_to_current(int32_t adc);

typedef enum motor_direction_t { MOTOR_FWD, MOTOR_REV } motor_direction_t;

#endif /* __MOTOR_H */