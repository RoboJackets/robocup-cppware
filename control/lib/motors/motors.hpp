#pragma once

#include <Arduino.h>

// From front perspective
enum Motors {
    FR_MOTOR,
    BR_MOTOR,
    BL_MOTOR,
    FL_MOTOR,
    DRIBBLER,
    MOTOR_COUNT,
};

/// @brief State handler for one motorboard motor
class MotorController {
private:
    HardwareSerial& uart;
    uint32_t baud;

public:
    /// @brief Constructor for motor controller
    /// @param motor_uart Serial to be used to communicate with motor
    /// @param baud Serial baud rate
    MotorController(HardwareSerial& motor_uart, uint32_t baud = 115200);

    /// @brief Start serial connection with motor
    void begin();

    /// @brief Send command to motor
    /// @param setpoint Target ticks per second motor velocity
    void send_command(int32_t setpoint);

    /// @brief Read velocity response from motor
    /// @return Motor velocity in ticks per second
    int32_t read_current_velocity();

    /// @brief Safely performs both a read and write to motor
    /// @param setpoint Target ticks per second motor velocity
    /// @return Value returned by motor
    int32_t send_and_read(int32_t setpoint);
};