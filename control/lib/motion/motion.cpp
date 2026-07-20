#include "motion.hpp"

MotionControl::MotionControl() {
    float wheel_angles[4] = {
        radians((180.0 - FRONT_ANGLE)),
        radians((180.0 + BACK_ANGLE)),
        radians((360.0 - BACK_ANGLE)),
        radians((0.0 + FRONT_ANGLE)),
    };

    bot_to_wheel(0, 0) = sinf(wheel_angles[0]);
    bot_to_wheel(0, 1) = -cosf(wheel_angles[0]);
    bot_to_wheel(0, 2) = -WHEEL_DIST;
    bot_to_wheel(1, 0) = sinf(wheel_angles[1]);
    bot_to_wheel(1, 1) = -cosf(wheel_angles[1]);
    bot_to_wheel(1, 2) = -WHEEL_DIST; 
    bot_to_wheel(2, 0) = sinf(wheel_angles[2]);
    bot_to_wheel(2, 1) = -cosf(wheel_angles[2]);
    bot_to_wheel(2, 2) = -WHEEL_DIST;
    bot_to_wheel(3, 0) = sinf(wheel_angles[3]);
    bot_to_wheel(3, 1) = -cosf(wheel_angles[3]);
    bot_to_wheel(3, 2) = -WHEEL_DIST;

    Matrix<float, 3, 4> bot_to_wheels_t = bot_to_wheel.transpose();
    
    wheel_to_bot = (bot_to_wheels_t * bot_to_wheel).inverse() * bot_to_wheels_t;

    last_state = Vector3f::Zero();
    last_imu = Vector3f::Zero();
    last_target_velocity = Vector3f::Zero();
    wheel_correction = Vector4f::Zero();
    timesteps = STABILIZE_TIME;
}

Vector4i MotionControl::body_to_wheels(Vector3f body_velocity) {
    Vector4f wheel_velocities = bot_to_wheel * body_velocity;
    Vector4i ticks;
    for (size_t i = 0; i < 4; i++) {
        ticks(i) = meters_to_ticks(wheel_velocities(i));
    }
    return ticks;
}

Vector3f MotionControl::wheels_to_body(Vector4f wheel_velocity) {
    return wheel_to_bot * wheel_velocity;
}

Vector4i MotionControl::control_update(Vector3f imu_measurements, Vector4i encoder_velocities, Vector3f target_velocity, uint32_t delta) {
    // Ensure 0 is 0
    if (target_velocity == Vector3f::Zero()) {
        return Vector4i::Zero();
    }

    // Reset new state timestamps
    if (target_velocity != last_target_velocity) {
        timesteps = 0;
    }

    // Find state estimates
    Vector3f estimated_imu = imu_estimate(imu_measurements, delta);
    Vector3f estimated_encoder = encoder_estimate(encoder_velocities);

    Vector3f state_estimate;
    if (imu_measurements(0) * imu_measurements(0) + imu_measurements(1) * imu_measurements(1) > HIGH_ACCELERATION_CUTOFF) {
        state_estimate = estimated_imu * ALPHA + estimated_encoder * (1 - ALPHA);
    } else {
        state_estimate = estimated_imu * HIGH_ALPHA + estimated_encoder * (1 - HIGH_ALPHA);
    }

    Vector3f difference = (target_velocity - state_estimate) * CORRECT_FACTOR;

    // Find the update velocity
    Vector4f update_velocity = bot_to_wheel * difference;

    // Update last measurements
    last_state = state_estimate;
    last_imu = imu_measurements;

    if (timesteps >= STABILIZE_TIME) {
        wheel_correction += update_velocity;
    }

    // Return new wheel velocities
    Vector4f wheel_velocities = bot_to_wheel * target_velocity + wheel_correction;
    Vector4i ticks;
    for (size_t i = 0; i < 4; i++) {
        ticks(i) = meters_to_ticks(wheel_velocities(i));
    }
    return ticks;
}

Vector3f MotionControl::imu_estimate(Vector3f imu_measurements, uint32_t delta) {
    float delta_f = delta / 1000000.0;
    Vector3f estimate {
        delta_f * (imu_measurements(0) - last_imu(0)) + last_state(0),
        delta_f * (imu_measurements(1) - last_imu(1)) + last_state(1),
        imu_measurements[2]
    };
    return estimate;
}

Vector3f MotionControl::encoder_estimate(Vector4i encoder_velocities) {
    Vector4f meters;
    for (size_t i = 0; i < 4; i++) {
        meters(i) = ticks_to_meters(encoder_velocities(i));
    }
    return wheel_to_bot * meters;
}

inline float ticks_to_meters(int32_t value) {
    return float(value) / TICKS_PER_ROTATION * 4.0 * PI * WHEEL_RADIUS;
}

inline int32_t meters_to_ticks(float value) {
    return int(value / (4.0 * PI * WHEEL_RADIUS) * TICKS_PER_ROTATION);
}