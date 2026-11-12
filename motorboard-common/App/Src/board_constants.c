#include "board_constants.h"
#include "main.h"
#include "stm32g0xx_hal_gpio.h"
#include <stdbool.h>

/* CONSTANTS FOR DRIVE MOTORBOARD V1 */
static const analog_constants_t DRIVE_MOTORBOARD_V1_ANALOG_CONSTANTS = {
    .sense_gain = 0.1095f,
    .sense_offset = 1.65f,
    .sense_range_low = -15.07f,
    .sense_range_high = 15.07f,
    .oc_threshold = 10.05f};

/* CONSTANTS FOR DRIBBLER MOTORBOARD V1 */
// TODO: Fill this out when this board goes to production
static const analog_constants_t DRIBBLER_MOTORBOARD_V1_ANALOG_CONSTANTS = {
    .sense_gain = 0.0f,
    .sense_offset = 0.0f,
    .sense_range_low = 0.0f,
    .sense_range_high = 0.0f,
    .oc_threshold = 0.0f};

enum boardtype fetch_board_type(void) {
  uint8_t chipid_a = HAL_GPIO_ReadPin(CHIPID_A_GPIO_Port, CHIPID_A_Pin);
  uint8_t chipid_b = HAL_GPIO_ReadPin(CHIPID_B_GPIO_Port, CHIPID_B_Pin);

  int index = chipid_a | (chipid_b << 1);

  switch (index) {
  case 0:
    // this variant has both pulled low
    return BOARD_TYPE_DRIVE_V1;
  case 1:
    return BOARD_TYPE_UNKNOWN;
  case 2:
    return BOARD_TYPE_UNKNOWN;
  case 3:
    // this variant has both pulled high
    return BOARD_TYPE_DRIBBLER_v1;
  default:
    return BOARD_TYPE_UNKNOWN;
  }
}

enum boardtype get_board_type(void) {
  // We really only need to determine this once, as it cannot change.
  // (unless someone is actively cutting traces)
  static enum boardtype board_type = BOARD_TYPE_UNKNOWN;
  static bool fectched = false;
  if (!fectched) {
    board_type = fetch_board_type();
    fectched = true;
  }

  return board_type;
}

const analog_constants_t get_analog_constants(void) {
  switch (get_board_type()) {
  case BOARD_TYPE_DRIVE_V1:
    return DRIVE_MOTORBOARD_V1_ANALOG_CONSTANTS;
  case BOARD_TYPE_DRIBBLER_v1:
    return DRIBBLER_MOTORBOARD_V1_ANALOG_CONSTANTS;
  default:
    return (analog_constants_t){0};
  }
}