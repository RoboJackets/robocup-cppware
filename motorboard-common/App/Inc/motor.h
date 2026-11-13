#ifndef __MOTOR_H
#define __MOTOR_H

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

enum motor_phase_state_t {
  MOTOR_PHASE_HIGH = 0,
  MOTOR_PHASE_LOW = 1,
  MOTOR_PHASE_FLOAT = 2,
};

void motor_set_phase_state(enum motor_phase_state_t stateU,
                           enum motor_phase_state_t stateV,
                           enum motor_phase_state_t stateW);

float motor_read_current(void);

#endif /* __MOTOR_H */