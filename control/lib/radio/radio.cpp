#include "radio.hpp"

void RobotStatusMessage::pack(uint8_t (&pkg)[ROBOT_STATUS_SIZE]) {
    // Ensure clean buffer
    memset(pkg, 0, ROBOT_STATUS_SIZE);
    pkg[0] |= team << 7;
    pkg[0] |= (robot_id & 0b1111) << 3;
    pkg[0] |= ball_sense_status << 2;
    pkg[0] |= kick_status << 1;
    pkg[0] |= kick_healthy;
    pkg[1] = battery_percent;
    pkg[2] |= (motor_errors & 0b11111) << 3;
    pkg[2] |= fpga_status << 2;
}

void ControlMessage::unpack(uint8_t (&data)[CONTROL_MESSAGE_SIZE]) {
    team = ((data[0] & (0b1 << 7)) ? Team::Yellow : Team::Blue);
    robot_id = (data[0] & (0b1111 << 3)) >> 3;
    shoot_mode = ((data[0] & 0b1 << 2) ? ShootMode::Chip : ShootMode::Kick);
    switch (data[0] & 0b11) {
        case 1: trigger_mode = TriggerMode::Immediate; break;
        case 2: trigger_mode = TriggerMode::Breakbeam; break;
        default: trigger_mode = TriggerMode::Disabled; break;
    }
    body_x = data[1] | (data[2] << 8);
    body_y = data[3] | (data[4] << 8);
    body_w = data[5] | (data[6] << 8);
    dribbler_speed = data[7];
    kick_strength = data[8];
    role = (data[9] & (0b11 << 6)) >> 6;
    mode = (data[9] & 0b00111111);
}

Vector3f ControlMessage::get_velocity() {
    Vector3f velocities {
        body_x / VELOCITY_SCALE_FACTOR,
        body_y / VELOCITY_SCALE_FACTOR,
        body_w / VELOCITY_SCALE_FACTOR
    };
    return velocities;
}

const String ControlMessage::to_string() {
    return "Team: " + String((team == Blue ? "Blue" : "Yellow")) + String(" | ID: ") + robot_id + " | Shoot Mode: " + shootmode_to_str(shoot_mode) + " | Trigger Mode: " + triggermode_to_str(trigger_mode) + " | X: " + body_x + " | Y: " + body_y + " | W: " + body_w + " | Dribbler Speed: " + dribbler_speed + " | Kick Strength " + kick_strength + " | Role: " + role + " | Mode: " + mode;
}

uint8_t acks_to_percent(bool (&acks)[100]) {
    uint8_t percent = 0;
    for (size_t i = 0; i < 100; i++) {
        if (acks[i]) percent += 1;
    }
    return percent;
}