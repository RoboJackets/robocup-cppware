#pragma once

#include <Arduino.h>

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
    /// @param setpoint New setpoint for motor (TODO: Figure out what this means)
    void send_command(int32_t setpoint);

    /// @brief Read velocity response from motor
    /// @return Value returned by motor (TODO: Figure out what this means)
    int32_t read_current_velocity();

    /// @brief Safely performs both a read and write to motor
    /// @param setpoint Mew setpoint for motor
    /// @return Value returned by motor
    int32_t send_and_read(int32_t setpoint);
};