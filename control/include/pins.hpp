// Pin map for teensy pinout
// Try to keep up to date with ref doc https://docs.google.com/spreadsheets/d/1R4ADXmqaLGxfWDbyh1WTpnZDjxXoTz-bSd3AXz-UCJA/edit?usp=sharing

#pragma once

// Motor Board //
#define MOTOR_EN_PIN 23
#define KILLN_PIN 36
#define POWER_SWITCH_PIN 40

// Team select //
#define BOTSEL_PIN0 35
#define BOTSEL_PIN1 34
#define BOTSEL_PIN2 33
#define BOTSEL_PIN3 32

// Kicker //
#define KICKER_MISO_PIN 39
#define KICKER_CSN_PIN 38
#define KICKER_RESETN_PIN 37

// Radio //
#define RADIO_CSN_PIN 10
#define RADIO_CE_PIN 41
#define RADIO_IRQ_PIN 9

// Batt Sense //
#define BATTERY_SENSE_PIN 15