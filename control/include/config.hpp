#pragma once

#define DEBUG 0

/// Battery safety checks
// At 2.372V, our batteries have depleted to 18.5V (see voltage divider in schematics)
#define MIN_BATTERY_VOLTAGE 2.384615
// Maximum Voltage of batteries is roughly 2.69
#define MAX_BATTERY_VOLTAGE 2.69
// Number of times the battery voltage can be measured under min before suicide
#define BATT_UVLO_THRESHOLD 10