/*
Pin map for teensy pinout
Try to keep up to date with ref doc https://docs.google.com/spreadsheets/d/1R4ADXmqaLGxfWDbyh1WTpnZDjxXoTz-bSd3AXz-UCJA/edit?usp=sharing
Not all pins are used currently, but for tracking sake all will be defined
*/

#pragma once

// Motor Board //
#define MOTOR_EN_PIN 23
#define KILLN_PIN 36
#define POWER_SWITCH_PIN 40
#define MOTOR_RESET_PIN 5
#define MOTOR_DIAG_EN_PIN 17 // Unknown
#define MOTOR_SNS_PIN 16 // Unknown

// Serial1
#define MOTOR_1_TX 0
#define MOTOR_1_RX 1
#define MOTOR_1_PROG 2

// Serial2
#define MOTOR_2_TX 7
#define MOTOR_2_RX 8
#define MOTOR_2_PROG 6

// Serial6
#define MOTOR_3_TX 25
#define MOTOR_3_RX 24
#define MOTOR_3_PROG 31

// Serial7
#define MOTOR_4_TX 28
#define MOTOR_4_RX 29
#define MOTOR_4_PROG 30

// Serial5
#define MOTOR_5_TX 21
#define MOTOR_5_RX 20
#define MOTOR_5_PROG 22

// Team select //
#define BOTSEL_PIN0 35
#define BOTSEL_PIN1 34
#define BOTSEL_PIN2 33
#define BOTSEL_PIN3 32

// Kicker //
#define KICKER_MISO_PIN 39
#define KICKER_MOSI_PIN 26
#define KICKER_CLK_PIN 27
#define KICKER_CSN_PIN 38
#define KICKER_RESETN_PIN 37

// Radio //
#define RADIO_MISO_PIN 12
#define RADIO_MOSI_PIN 11
#define RADIO_CLK_PIN 13
#define RADIO_CSN_PIN 10
#define RADIO_CE_PIN 41
#define RADIO_IRQ_PIN 9

// Batt Sense //
#define BATTERY_SENSE_PIN 15

// Display //
#define DISPLAY_SCL 19
#define DISPLAY_SDA 18