/*
Motion Control Module for controlling the commands sent to move the motors on the robot
Currently only used for body to wheel velocity conversion as PID was iffy
*/

#pragma once

#include <ArduinoEigen.h>
using namespace Eigen;

/// Physical dimensions

// Wheel radius (m)
#define WHEEL_RADIUS 0.02786
// Rear wheel distance to center (m) ((NEED CONFIRM))
#define REAR_WHEEL_DIST 0.077874
// Front wheel distance to center (m) ((NEED CONFIRM))
#define FRONT_WHEEL_DIST 0.078089
// Distance between wheels (m)
#define WHEEL_DIST (FRONT_WHEEL_DIST + REAR_WHEEL_DIST) / 2.0
// Angle of front wheels (Degrees?)
#define FRONT_ANGLE 30.0
// Angle of back wheels (Degrees?)
#define BACK_ANGLE 45.0
// The number of encoder ticks per rotation (Wheels have a gear ration of 1:3 and encoders have 2048 counts per rotation)
#define TICKS_PER_ROTATION 6144.0

/// Tuning

// Weighting for IMU sensor readings
#define ALPHA 0.15
// Weighting for IMU sensor readings at high acceleration
#define HIGH_ALPHA 0.8
// High Acceleration Cutoff (squared) (We should weight the accelerometer higher if there is a high acceleration)
#define HIGH_ACCELERATION_CUTOFF 0.5 * 0.5
// Factor with which to correct headings by
#define CORRECT_FACTOR 0.5
// Number of timesteps to wait for measurements to stabilize
#define STABILIZE_TIME 2



struct MotionControl {
    // Bot to wheel velocity conversion matrix
    Matrix<float, 4, 3> bot_to_wheel;
    // Wheel to bot velocity conversion matrix
    Matrix<float, 3, 4> wheel_to_bot;
    // Last state estimate {v_xt, y_xt, w_t} (m/s, m/s, rad/s)
    Vector3f last_state;
    // Last IMU measurement {a_xt, a_yt, w_t} (m/s^2, m/s^2, rad/s)
    Vector3f last_imu;
    // Last target velocity {v_xt, y_xt, w_t} (m/s, m/s, rad/s)
    Vector3f last_target_velocity;
    // Correction value per each wheel (m/s)
    Vector4f wheel_correction;
    // Number of timesteps at the current target velocity
    uint32_t timesteps;

    /**
     * Motion manager constructor
     */
    MotionControl();

    /**
     * Convert movement velocities into individual wheel velocities (ticks/s)
     * 
     * @param body_velocity {x, y, w} (m/s, m/s, rad/s)
     * 
     * @return {motor_1, motor_2, motor_3, motor_4} (ticks/s)
     */
    Vector4i body_to_wheels(Vector3f body_velocity);

    /**
     * Convert wheel velocities to movement velocities
     * 
     * @param body_velocity {motor_1, motor_2, motor_3, motor_4} (ticks/s)
     * 
     * @return {x, y, w} (m/s, m/s, rad/s)
     */
    Vector3f wheels_to_body(Vector4f);

    /**
     * Control update
     * 
     * @param imu_measurements {a_xt, a_yt, w_t} (m/s^2, m/s^2, rad/s)
     * @param encoder_velocities {v1_t, v2_t, v3_t, v4_t} (tick/s)
     * @param target_velocity {x, y, w} (m/s, m/s, rad/s)
     * @param delta elapsed time (us)
     * 
     * @return {motor_1, motor_2, motor_3, motor_4} (ticks/s)
     */
    Vector4i control_update(Vector3f imu_measurements, Vector4i encoder_velocities, Vector3f target_velocity, uint32_t delta);

    /**
     * Calculate the current state estimate based on IMU measurements
     * 
     * @param imu_measurements {a_xt, a_yt, w_t} (m/s^2, m/s^2, rad/s)
     * @param delta elapsed time (us)
     * 
     * @return velocity estimate {v_xt, y_xt, w_t} (m/s, m/s, rad/s)
     */
    Vector3f imu_estimate(Vector3f imu_measurements, uint32_t delta);

    /**
     * Calculate the current state estimate based on encoder velocities
     * 
     * @param encoder_velocities {v1_t, v2_t, v3_t, v4_t} (tick/s)
     * 
     * @return velocity estimate {v_xt, v_yt, w_t} (m/s, m/s, rad/s)
     */
    Vector3f encoder_estimate(Vector4i encoder_velocities);
};


/**
 * Convert ticks/s to m/s
 * 
 * @param value ticks/s
 */
float ticks_to_meters(int32_t);

/**
 * Convert m/s to ticks/s
 * 
 * @param value m/s
 */
int32_t meters_to_ticks(float);