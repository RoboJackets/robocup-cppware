#ifndef __BOARD_CONSTANTS_H
#define __BOARD_CONSTANTS_H

/**
 * The Drive MotorBoard and Dribbler MotorBoard share the same codebase.
 * They are designed to use exactly the same pinouts and MCU (STSPIN32G0B2),
 * and only differ with analog circuitry and external sensors.
 * This header allows the correct constant to be fetched at runtime.
 */

enum boardtype {
  BOARD_TYPE_UNKNOWN,
  BOARD_TYPE_DRIVE_V1,
  BOARD_TYPE_DRIBBLER_v1
};

enum boardtype get_board_type(void);

typedef struct {
  float sense_gain;       // Current Gain (V/A), Volts/Amps
  float sense_offset;     // Voltage offset, Volts
  float sense_range_low;  // Lower current sense limit, Amps
  float sense_range_high; // Upper current sense limit, Amps
  float oc_threshold;     // Overcurrent threshold, Amps
} analog_constants_t;

const analog_constants_t get_analog_constants(void);

#endif // __BOARD_CONSTANTS_H