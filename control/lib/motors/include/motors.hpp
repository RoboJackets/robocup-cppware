#pragma once

#include <Arduino.h>

class MotorController {
private:
    HardwareSerial& uart;
    uint32_t baud;

public:
    MotorController(HardwareSerial& motor_uart, uint32_t baud = 115200);

    void begin();

    void send_command(int32_t setpoint);

    int32_t read_current_velocity();
};