/*
Motor driver to control the four body motors and dribbler via UART
*/

#pragma once

#include <Arduino.h>

// From front perspective
// Use to index motors array
// Probably useless but fun to exist
enum Motors {
    FR_MOTOR,
    BR_MOTOR,
    BL_MOTOR,
    FL_MOTOR,
    DRIBBLER,
    MOTOR_COUNT,
};

// State handler for one motorboard motor
class MotorController {
private:
    HardwareSerial& _uart;
    uint32_t _baud;

public:
    /**
     * Constructor for motor controller
     * 
     * @param motor_uart Serial to be used to communicate with motor
     * @param baud Serial baud rate (defaults to 115200)
     */
    MotorController(HardwareSerial& motor_uart, uint32_t baud = 115200);

    /**
     * Start serial connection with motor
     */
    void begin();

    /**
     * Send command to motor
     * 
     * @param setpoint Target ticks per second motor velocity
     */
    void send_command(int32_t setpoint);

    /**
     * Read velocity response from motor
     * 
     * @return Motor velocity in ticks per second
     */
    int32_t read_current_velocity();

    /**
     * Safely performs both a read and write to motor
     * 
     * @param setpoint Target ticks per second motor velocity
     * 
     * @return Motor velocity in ticks per second
     */
    int32_t send_and_read(int32_t setpoint);
};