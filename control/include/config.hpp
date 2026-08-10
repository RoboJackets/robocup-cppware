#pragma once

#define DEBUG 0

/// Battery safety checks
// At 2.372V, our batteries have depleted to 18.5V (see voltage divider in schematics)
#define MIN_BATTERY_VOLTAGE 2.384615
// At 2.244V, our batteries have depleted to 17.5V, this should be the absolute lowest voltage
#define UNSAFE_BATTERY_VOLTAGE 2.243784
// Maximum Voltage of batteries is roughly 2.69
#define MAX_BATTERY_VOLTAGE 2.69
// Number of times the battery voltage can be measured under min before suicide
#define BATT_UVLO_THRESHOLD 10
// Amount of time robot should keep moving without new command
#define DIE_TIME_MS 1000
// Frequency of the interval timer for motor control
#define MOTION_FREQ_US 1000000 / 60 // 60hz
// Frequency of the interval timer for kicker control
#define KICKER_FREQ_US 1000000 / 30 // 30hz
// Frequency of the interval timer for low priority tasks (ex: screen update, battery reading)
#define LOW_PRIO_FREQ_US 1000000 / 2 // 2hz
// Number of robots max
#define NUM_BOTS 6