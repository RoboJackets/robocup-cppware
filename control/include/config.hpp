#pragma once

#define DEBUG 1

/// Battery safety checks
// At 2.372V, our batteries have depleted to 18.5V (see voltage divider in schematics)
#define MIN_BATTERY_VOLTAGE 2.384615
// Maximum Voltage of batteries is roughly 2.69
#define MAX_BATTERY_VOLTAGE 2.69
// Number of times the battery voltage can be measured under min before suicide
#define BATT_UVLO_THRESHOLD 10
// Amount of time robot should keep moving without new command
#define DIE_TIME_MS 1000

#define CONTROL_FREQ_MS 1000 / 60