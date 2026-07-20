#include "motors.hpp"


MotorController::MotorController(HardwareSerial& motor_uart, uint32_t baud)
    : uart(motor_uart), baud(baud) {}

void MotorController::begin() {
    uart.begin(baud);
}

void MotorController::send_command(int32_t setpoint) {
    uart.write(0x11); // Required additional byte due to legacy
    uart.write((uint8_t*)&setpoint, 4); // Break setpoint into 4 LE ordered bytes
}

int32_t MotorController::read_current_velocity() {
    int32_t velocity = 0;
    if (uart.available() >= 4) {
        uint8_t buffer[4];
        uart.readBytes(buffer, 4);
        
        // Reconstruct little-endian int32_t
        velocity = (int32_t)(buffer[0] | 
                           (buffer[1] << 8) | 
                           (buffer[2] << 16) | 
                           ((int32_t)buffer[3] << 24));
    }
    return velocity;
}

int32_t MotorController::send_and_read(int32_t setpoint) {
    send_command(setpoint);
    // Small delay to allow velocity data to be packed, check if needed/can be safely replaced with serial available check
    return read_current_velocity();
}