#include "encoder.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_tim.h"
#include "tim.h"

static uint32_t last_pos = 0;
static uint32_t last_time = 0;
static volatile int32_t last_vel = 0;

// Intended to be called from TIM14 ISR
void velocity_update(void) {
  // Encoder delta via unsigned wraparound (TIM2 is 32-bit, no reset needed)
  uint32_t pos = __HAL_TIM_GET_COUNTER(&htim2);
  int32_t delta = (int32_t)(pos - last_pos);
  last_pos = pos;

  // Actual elapsed ticks since last call (nominally 16000 at 64 MHz)
  uint32_t current_time = __HAL_TIM_GET_COUNTER(&htim14);
  uint32_t delay = 16000U - last_time + current_time;
  last_time = current_time;

  // vel = delta * 64e6 / delay = delta * 4000 * (16000 / delay)
  uint32_t ratio_q16 = (16000UL << 16) / delay;
  last_vel = (int32_t)(((int64_t)delta * 4000 * ratio_q16) >> 16);
}

int32_t velocity_get(void) { return last_vel; }