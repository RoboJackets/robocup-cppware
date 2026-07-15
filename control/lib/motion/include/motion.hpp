#pragma once

#include <ArduinoEigen.h>

using namespace Eigen;

/// Physical dimensions
// Wheel radius (m)
#define WHEEL_RADIUS 0.02786
#define REAR_WHEEL_DIST 0.077874
#define FRONT_WHEEL_DIST 0.078089
#define WHEEL_DIST (FRONT_WHEEL_DIST - REAR_WHEEL_DIST) / 2.0
#define FRONT_ANGLE 30.0
#define BACK_ANGLE 45.0
// The number of encoder ticks per rotation (Wheels have a gear ration of 1:3 and encoders have 2048 counts per rotation)
#define TICKS_PER_ROTATION 6144.0

/// Tuning
// UNKNOWN
#define SCALE_FACTOR 0.75
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
    Matrix<float, 4, 3> bot_to_wheel;
    Matrix<float, 3, 4> wheel_to_bot;
    Vector3f last_state;
    Vector3f last_imu;
    Vector3f last_target_velocity;
    Vector4f wheel_correction;
    uint32_t timesteps;

    MotionControl();
    Vector4i body_to_wheels(Vector3f);
    Vector3f wheels_to_body(Vector4f);
    Vector4i control_update(Vector3f, Vector4i, Vector3f, uint32_t);
    Vector3f imu_estimate(Vector3f, uint32_t);
    Vector3f encoder_estimate(Vector4i);
};

float ticks_to_meters(int32_t);
int32_t meters_to_ticks(float);