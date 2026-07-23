#pragma once

#define DEBUG 0
#define EXTRA_INFO 0 // Enables extra debug statements outside of main control loop
#define DISABLE_ERRORS 0 // Purely for sofware debugging, do not use
#define SPI_CLK_FREQUENCY 2000000 // Hz
#define BTN_COOLDOWN 500 // ms
#define WATCHDOG_TIMEOUT 25000 // ms time till kick command cancelled

#define CHARGE_SYNC_EN 1 // Do not set if board does not have charge sync resistor
#define CHARGE_SYNC_FREQ 100000 // Hz (DONT GO OVER 200KHZ WILL DESTROY BOARD)

#define VOLT_RANGE 3.3 / 4096.0 // Pico Vref vs ADC
#define VOLT_CONVERSION 250 / 2.025 * VOLT_RANGE // Voltage scale with divider values
#define VOLT_MAX 180
#define VOLT_MIN 10
#define KALPHA_VOLT 64 // KALHPA / 255 of last value used for averaging | Max 255
#define VOLT_HIST_SIZE 20 // Decides how far back can be checked: SIZE * 25ms (check timing if changed)
#define SHORT_HIST_DIFF 5 // How far back to look for short diff in voltage (val * 25ms)
#define LONG_HIST_DIFF 20 // How far back to look for long diff in voltage (val * 25ms)


#define MAX_KICK_TIME 28200 // us
#define KICK_COOLDOWN 200 // ms
#define CHARGE_COOLDOWN 100 // ms

#define BREAK_THRESHOLD 400 // Max difference to trigger breakbeam
#define KALPHA_BREAK 128 // Larger = more responive but more sensitive to light | Max 255

// Error checking
#define CHARGE_TIME_MAX 20000 // ms
#define OVER_VOLTAGE 210
#define VERY_OVER_VOLTAGE 250
#define VOLT_TOLERANCE 2 // Used for comparing to old_voltage
#define VOLT_TOLERANCE_CHARGE 10
#define VOLT_TOLERANCE_KICK 5 // Minumum required volt drop for kick
#define NO_CHARGE_COOLDOWN 500 // ms
#define CONTINUOUS_CHARGE_COOLDOWN 500 // ms
#define LARGEST_PASSIVE_DROP 3 // (v/500ms) Casual observation shows normally around 2v/s, so 6v/s is generous

// Enables for specific errors
#define E_OVER_VOLTAGE 1
#define E_MAJOR_OVER_VOLTAGE 1
#define E_CONTINUOUS_CHARGING 1
#define E_CONTINUOUS_DISCHARGING 1
#define E_NO_CHARGE 1
#define E_CHARGE_TIMEOUT 1
#define E_NO_DISCHARGE 1
#define E_BREAK_BLOCKAGE 1