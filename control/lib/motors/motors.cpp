#include "motors.hpp"


MotorController::MotorController(HardwareSerial& motor_uart, uint32_t baud)
    : _uart(motor_uart), _baud(baud) {}

void MotorController::begin() {
    _uart.begin(_baud);
}

void MotorController::send_command(int32_t setpoint) {
    _uart.write(0x11); // Required additional byte due to legacy (Maybe for alignment?)
    _uart.write((uint8_t*)&setpoint, 4); // Break setpoint into 4 LE ordered bytes by converting to array pointer
}

int32_t MotorController::read_current_velocity() {
    int32_t velocity = 0;
    if (_uart.available() >= 4) {
        uint8_t buffer[4];
        _uart.readBytes(buffer, 4);
        
        // Reconstruct little-endian int32_t
        velocity = (int32_t)(buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | ((int32_t)buffer[3] << 24));
    }
    return velocity;
}

int32_t MotorController::send_and_read(int32_t setpoint) {
    send_command(setpoint);
    // Delay might be needed but for now seems stable as is
    return read_current_velocity();
}