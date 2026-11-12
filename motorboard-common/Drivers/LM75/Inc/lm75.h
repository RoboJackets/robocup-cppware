#ifndef __LM75_H
#define __LM75_H

#include "i2c_generic.h"

typedef struct {
  const i2c_bus_t *bus;
  uint8_t addr; // 7-bit addr
} lm75_t;

enum lm75_config_os_queue {
  LM75_OS_QUEUE1 = 0,
  LM75_OS_QUEUE2 = 1,
  LM75_OS_QUEUE4 = 2,
  LM75_OS_QUEUE6 = 3
};
enum lm75_config_os_polarity { LM75_OS_POL_LOW = 0, LM75_OS_POL_HIGH = 1 };
enum lm75_config_os_mode { LM75_OS_COMPARATOR = 0, LM75_OS_INTERRUPT = 1 };
enum lm75_config_mode { LM75_MODE_NORMAL = 0, LM75_MODE_SHUTDOWN = 1 };

typedef struct {
  enum lm75_config_os_queue os_queue;
  enum lm75_config_os_polarity os_polarity;
  enum lm75_config_os_mode os_mode;
  enum lm75_config_mode mode;
} lm75_config_t;

enum lm75_register {
  LM75_REG_CONFIG = 0x01,
  LM75_REG_TEMP = 0x00,
  LM75_REG_OT = 0x02,
  LM75_REG_HYST = 0x03
};

/**
 * @brief Configure an LM75 sensor.
 *
 * @param sensor Instance of lm75_t
 * @param config Instance of lm75_config_t
 */
void lm75_set_config(const lm75_t *sensor, const lm75_config_t *config);

/**
 * @brief Read the configuration from an LM75 sensor.
 *
 * @param sensor Instance of lm75_t
 * @return lm75_config_t
 */
lm75_config_t lm75_get_config(const lm75_t *sensor);

/**
 * @brief Read the temperature from an LM75 sensor.
 *
 * @param sensor Instance of lm75_t.
 * @return float Temperature in degrees Celsius.
 */
float lm75_read_temperature(const lm75_t *sensor);

/**
 * @brief Set the overtemperature threshold on an LM75 sensor.
 *
 * @param sensor Instance of lm75_t.
 * @param temperature Overtemperature threshold in degrees Celsius.
 */
void lm75_set_overtemp(const lm75_t *sensor, float temperature);

/**
 * @brief Get the overtemperature threshold on an LM75 sensor.
 *
 * @param sensor Instance of lm75_t.
 * @return float Overtemperature threshold in degrees Celsius.
 */
float lm75_get_overtemp(const lm75_t *sensor);

/**
 * @brief Set the overtemperature hysteresis on an LM75 sensor.
 *
 * @param sensor Instance of lm75_t
 * @param temperature Overtemperature hysteresis threshold in degrees Celsius.
 */
void lm75_set_hysteresis(const lm75_t *sensor, float temperature);

/**
 * @brief Get the overtemperature hysteresis on an LM75 sensor.
 *
 * @param sensor Instance of lm75_t.
 * @return float Overtemperature hysteresis threshold in degrees Celsius.
 */
float lm75_get_hysteresis(const lm75_t *sensor);

#endif /* __LM75_H */