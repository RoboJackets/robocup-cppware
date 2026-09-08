/*
Various data types and helper functions used for radio communication
*/

#pragma once

#include <Arduino.h>
#include <ArduinoEigen.h>
using namespace Eigen;

#include "types.hpp"
#include "kicker.hpp"

// Radio channel to be used by NRF24
#define CHANNEL 104
// Size of control messages from base station
#define CONTROL_MESSAGE_SIZE 10
// Size of robot response message
#define ROBOT_STATUS_SIZE 3

// The body{X, Y, W} are multiplied (upon sending) by the VELOCITY_SCALE_FACTOR and divided
// (upon receiving) to preserve at least 3 decimals of floating point precision.
#define VELOCITY_SCALE_FACTOR 1000.0f

// The different possible base stations.
// 
// BASE_STATION_ADDRESSES[0] = Blue Team
// BASE_STATION_ADDRESSES[1] = Yellow Team
static const unsigned char BASE_STATION_ADDRESSES[2][5] = {
    {0xE7, 0xE7, 0xE7, 0xE7, 0xE7},
    {0xA4, 0xA4, 0xA4, 0xA4, 0xA4},
};

// The address for each robot
// 
// ROBOT_RADIO_ADDRESSES[0][X] = Blue Team Robot X Address
// ROBOT_RADIO_ADDRESSES[1][X] = Yellow Team Robot X Address
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

/**
 * The Robot Status Message is sent back from the robot's whenever they receive communication
 * to let software know that they are doing good.
 *
 * The RobotStatusMessage has the following format:
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | team    | robot_id                              | b_sense | k_status| k_health|
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | battery_voltage                                                               |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | motor_errors                                    | fpga_s  | unused            |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 *
 * Size = 3 Bytes
 */
struct RobotStatusMessage {
    // Team of robot (0 blue : 1 yellow)
    Team team = Team::Blue;
    // ID of robot
    uint8_t robot_id = 0;
    // True if breakbeam currently triggered
    bool ball_sense_status = false;
    // True if a kick is queued
    bool kick_status = false;
    // True if kicker healthy
    bool kick_healthy = false;
    // Percent battery 0-100
    uint8_t battery_percent = 0;
    // Motor errors (Not implemented)
    uint8_t motor_errors = 0;
    // Status of FPGA (Legacy)
    bool fpga_status = false;

    /**
     * Packs the current status into SPI format for radio
     * 
     * @param pkg Buffer to pack status into, must be of `ROBOT_STATUS_SIZE` size
     */
    void pack(uint8_t (&pkg)[ROBOT_STATUS_SIZE]);
};

/**
 * The Control Message is Sent from the Base Station to the Robots.
 *
 * The Packed Format of this message is as follows:
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | team    | robot id                              | shoot_m | trigger_mode      |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | body_x (lsb)                                                                  |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | body_x (msb)                                                                  |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | body_y (lsb)                                                                  |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | body_y (msb)                                                                  |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | body_w (lsb)                                                                  |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | body_w (msb)                                                                  |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | dribbler_speed                                                                |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | kick_strength                                                                 |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 * | role              | mode                                                      |
 * +---------+---------+---------+---------+---------+---------+---------+---------+
 *
 * Size = 80 Bits = 10 Bytes
 */
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
    // Role of robot (Legacy)
    uint8_t role = 0;
    // Mode, 0 is normal, rest are for debug (Legacy?)
    uint8_t mode = 0;

    /**
     * Updates the control message values from radio SPI control message
     * 
     * @param data Raw data from radio of `CONTROL_MESSAGE_SIZE` size
     */
    void unpack(uint8_t (&data)[CONTROL_MESSAGE_SIZE]);

    /**
     * Formats velocity data from control message
     * 
     * @return Properly formatted velocities
     * 
     * @warning Do not read the values straight from the struct, use this function
     * otherwise scaling will not be applied.
     */
    Vector3f get_velocity();

    /**
     * Converts control message into a readible String format
     * 
     * @return String representation of control message
     */
    const String to_string();
};

/**
 * Converts an array of acks into a percent rate
 * 
 * @param acks 100 index array of bools where true means good send
 * 
 * @return Percent rate of good sends 0-100
 */
uint8_t acks_to_percent(bool (&acks)[100]);