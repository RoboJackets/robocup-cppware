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
#include "stm32g0xx_hal_uart.h"
#include "temp.h"
#include "tim.h"
#include "usart.h"
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

static volatile uint32_t velocity = 0;
static volatile float current = 0;

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

  uint8_t spi_buf[32];
  static const uint8_t reply[5] = {1, 2, 3, 4, 5};

  while (1) {
    memset(spi_buf, 0, sizeof(spi_buf));

    HAL_SPI_Abort(&hspi1);        // stop any HAL transfer in progress
    __HAL_RCC_SPI1_FORCE_RESET(); // clears both FIFOs and all state
    __HAL_RCC_SPI1_RELEASE_RESET();
    HAL_SPI_Init(&hspi1);

    int ret =
        HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)reply, spi_buf, 5, 1000);
    if (ret == HAL_OK) {
      HAL_GPIO_TogglePin(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
      log_info("tick!");
    }

    flush_logs();
  }
}

// 10kHz motion control loop
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

  // update the current estimate
  current = motor_adc_to_current(motor_adc_sum / 16);
}

// Called on hall edge update
void HALL_ISR() {
  static int8_t last_index = -1;
  motor_hall_state_t halls = get_motor_hall_state();
  int8_t index = halls_to_index(halls);

  if (index != last_index) {
    last_index = index;
    enum motor_phase_state_t *motor_phases = phase_table_fwd[index];
    // current peaks right before a phase switch
    if (index % 2 == 0) {
      HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_out, 2);
    }
    motor_set_phase_state(motor_phases[0], motor_phases[1], motor_phases[2]);
  }
}