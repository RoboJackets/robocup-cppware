#include "temp.h"
#include "adc.h"
#include "board_constants.h"
#include "i2c.h"
#include "i2c_generic.h"
#include "lm75.h"
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_adc.h"
#include <sys/_intsup.h>

/* I2C Devices */

static int i2c1_write(void *ctx, uint8_t addr, const uint8_t *data,
                      size_t len_) {
  I2C_HandleTypeDef *i2c = (I2C_HandleTypeDef *)ctx;
  if (HAL_I2C_Master_Transmit(i2c, addr << 1, (uint8_t *)data, len_,
                              HAL_MAX_DELAY) != HAL_OK) {
    return -1;
  }
  return 0;
}

static int i2c1_read(void *ctx, uint8_t addr, uint8_t *data, size_t len_) {
  I2C_HandleTypeDef *i2c = (I2C_HandleTypeDef *)ctx;
  if (HAL_I2C_Master_Receive(i2c, addr << 1, data, len_, HAL_MAX_DELAY) !=
      HAL_OK) {
    return -1;
  }
  return 0;
}

static const i2c_bus_t i2c1_bus = {
    .ctx = &hi2c1,
    .write = i2c1_write,
    .read = i2c1_read,
};

static lm75_t lm75 = {
    .bus = &i2c1_bus,
    .addr = 0x48, // address pins all tied to GND
};

/* Internal Die Temperature */

// VDDA is 3.3V
static uint32_t VDDA_MV = 3300U;

static float read_die_temp(void) {
  ADC_ChannelConfTypeDef cfg = {0};
  cfg.Channel = ADC_CHANNEL_TEMPSENSOR;
  cfg.Rank = ADC_REGULAR_RANK_1;
  cfg.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  HAL_ADC_ConfigChannel(&hadc1, &cfg);

  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 10);
  uint32_t raw = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  int32_t temp_c = __HAL_ADC_CALC_TEMPERATURE(VDDA_MV, raw, ADC_RESOLUTION_12B);
  return (float)temp_c;
}

temp_readings_t get_temperature_readings(void) {
  temp_readings_t results = {};

  // Read from the ambient temperature sensor
  results.ambient = lm75_read_temperature(&lm75);

  // Read from the internal ADC
  results.die = read_die_temp();

  // If we're a dribbler board, we also have an external thermistor
  // over an I2C ADC
  if (get_board_type() == BOARD_TYPE_DRIBBLER_v1) {

    // TODO: Placeholder untl chip is decided
    results.motor = 0.0;
  } else {
    results.motor = 0.0;
  }

  return results;
}