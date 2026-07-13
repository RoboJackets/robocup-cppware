#pragma once

#define CHANNEL 106
#define CONTROL_MESSAGE_SIZE 10
#define ROBOT_STATUS_SIZE 3

/// The different possible base stations.
/// 
/// BASE_STATION_ADDRESSES[0] = Blue Team
/// BASE_STATION_ADDRESSES[1] = Yellow Team
static const unsigned char BASE_STATION_ADDRESSES[2][5] = {
    {0xE7, 0xE7, 0xE7, 0xE7, 0xE7},
    {0xA4, 0xA4, 0xA4, 0xA4, 0xA4},
};

/// The address for each robot
/// 
/// ROBOT_RADIO_ADDRESSES[0][X] = Blue Team Robot X Address
/// ROBOT_RADIO_ADDRESSES[1][X] = Yellow Team Robot X Address
static const unsigned char ROBOT_RADIO_ADDRESSES[2][6][5] = {
    {
        {0xC3, 0xC3, 0xC3, 0xC3, 0xC1},
        {0xC3, 0xC3, 0xC3, 0xC3, 0xC2},
        {0xC3, 0xC3, 0xC3, 0xC3, 0xC3},
        {0xC3, 0xC3, 0xC3, 0xC3, 0xC4},
        {0xC3, 0xC3, 0xC3, 0xC3, 0xC5},
        {0xC3, 0xC3, 0xC3, 0xC3, 0xC6},
    },
    {
        {0xD5, 0xD5, 0xD5, 0xD5, 0xD1},
        {0xD5, 0xD5, 0xD5, 0xD5, 0xD2},
        {0xD5, 0xD5, 0xD5, 0xD5, 0xD3},
        {0xD5, 0xD5, 0xD5, 0xD5, 0xD4},
        {0xD5, 0xD5, 0xD5, 0xD5, 0xD5},
        {0xD5, 0xD5, 0xD5, 0xD5, 0xD6},
    }
};

struct RobotStatusMessage {
    // Team of robot (0 blue : 1 yellow)
    Team team = Team::Blue;
    // ID of robot
    uint8_t robot_id = 0;
    // Whether breakbeam is triggered or not
    bool ball_sense_status = false;
    // Status of kicker
    bool kick_status = false;
    // Health of kicker
    bool kick_healthy = false;
    // Voltage measured
    uint8_t battery_voltage = 0;
    // Motor errors
    uint8_t motor_errors = 0;
    // Status of FPGA
    bool fpga_status = false;

    void pack(uint8_t (&pkg)[ROBOT_STATUS_SIZE]) {
        pkg[0] |= team;
        pkg[0] |= (robot_id & 0b1111) << 3;
        pkg[0] |= ball_sense_status << 2;
        pkg[0] |= kick_status << 1;
        pkg[0] |= kick_healthy;
        pkg[1] = battery_voltage;
        pkg[2] |= (motor_errors & 0b11111) << 3;
        pkg[2] |= fpga_status << 2;
    }
};

struct ControlMessage {
    // Team of robot (0 blue : 1 yellow)
    Team team = Team::Blue;
    // ID of robot
    uint8_t robot_id = 0;
    // Kicking vs Chipping
    ShootMode shoot_mode = ShootMode::Kick;
    // Trigger for kick/chip
    TriggerMode trigger_mode = TriggerMode::Disabled;
    // X Coordinate of the Robot's Body Frame (multiplied by VELOCITY_SCALE_FACTOR and truncated)
    int16_t body_x = 0;
    // Y Coordinate of the Robot's Body Frame (multiplied by VELOCITY_SCALE_FACTOR and truncated)
    int16_t body_y = 0;
    // W Coordinate of the Robot's Body Frame (multiplied by VELOCITY_SCALE_FACTOR and truncated)
    int16_t body_w = 0;
    // Speed of dribber, scale unknown
    int8_t dribbler_speed = 0;
    // Kick strength 0-15
    uint8_t kick_strength = 0;
    // Role of robot
    uint8_t role = 0;
    // Mode, 0 is normal, rest are for debug
    uint8_t mode = 0;

    void unpack(uint8_t (&data)[CONTROL_MESSAGE_SIZE]) {
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

    String to_string() {
        return "Team: " + String((team == Blue ? "Blue" : "Yellow")) + String(" | ID: ") + robot_id + " | Shoot Mode: " + shootmode_to_str(shoot_mode) + " | Trigger Mode: " + triggermode_to_str(trigger_mode) + " | X: " + body_x + " | Y: " + body_y + " | W: " + body_w + " | Dribbler Speed: " + dribbler_speed + " | Kick Strength " + kick_strength + " | Role: " + role + " | Mode: " + mode;
    }
};