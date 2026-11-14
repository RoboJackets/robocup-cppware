#include "encoder.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_tim.h"
#include "tim.h"

static uint32_t last_time = 0;
static uint32_t last_vel = 0;

static const uint32_t I32_MAX = 2147483647U;

// Intended to be called from TIM14 ISR
void velocity_update() {

  // get the difference in encoder reading
  // the period is I32_MAX*2, so if we keep setting it to I32_MAX, we will never
  // actually hit the edges
  uint32_t current_pos = __HAL_TIM_GET_COUNTER(&htim2);
  __HAL_TIM_SET_COUNTER(&htim2, I32_MAX);
  uint32_t delta = current_pos - I32_MAX;

  // compensate for the delay between the last ISR call and now
  uint32_t current_time = __HAL_TIM_GET_COUNTER(&htim14);
  uint32_t delay = 64000 - last_time + current_time;
  last_time = current_time;

  // ticks are at 64 MHz
  last_vel = delta * ((64000000U) / delay);
}

int32_t velocity_get(void) { return last_vel; }