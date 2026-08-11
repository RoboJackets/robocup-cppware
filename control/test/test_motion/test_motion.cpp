#include <unity.h>
#include <Arduino.h>
#include <ArduinoEigen.h>
using namespace Eigen;

#include "motion.hpp"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

// 8 decimal precision is all that is needed
bool float_compare(float a, float b) {
    return (int(a * 100000000) == int(b * 100000000));
}

void test_bot_to_wheel() {
    MotionControl motion_control = MotionControl();

    Matrix<float, 4, 3> expected_matrix {
        {0.50000006f, 0.86602539f, -0.0779815f,},
        {-0.7071067f, 0.70710683f, -0.0779815f,},
        {-0.7071069f, -0.70710665f, -0.0779815f,},
        {0.5f, -0.86602539f, -0.0779815f,}
    };

    TEST_ASSERT_TRUE(motion_control.bot_to_wheel == expected_matrix);
}

void test_wheel_to_bot() {
    MotionControl motion_control = MotionControl();

    Matrix<float, 3, 4> expected_matrix {
        {0.41421351f, -0.41421348f, -0.41421357f, 0.41421351f,},
        {0.34641021f, 0.28284279f, -0.28284270f, -0.34641021f,},
        {-3.75593162f, -2.65584517f, -2.65584493f, -3.75593209f,},
    };

    TEST_ASSERT_TRUE(motion_control.wheel_to_bot == expected_matrix);
}

void test_ticks_to_meters() {
    TEST_ASSERT_TRUE(float_compare(ticks_to_meters(TICKS_PER_ROTATION), 1.0 * PI * 2.0 * 2.0 * WHEEL_RADIUS));
}

void test_ticks_to_meters_zero() {
    TEST_ASSERT_TRUE(float_compare(ticks_to_meters(0), 0.0f));
}

void test_ticks_to_meters_negative() {
    // Negative ticks (reverse rotation) should mirror the positive case
    TEST_ASSERT_TRUE(float_compare(ticks_to_meters(-TICKS_PER_ROTATION), -1.0 * PI * 2.0 * 2.0 * WHEEL_RADIUS));
}

void test_meters_to_ticks() {
    TEST_ASSERT_TRUE(meters_to_ticks(1.0 * PI * 2.0 * 2.0 * WHEEL_RADIUS) == TICKS_PER_ROTATION);
}

void test_meters_to_ticks_negative() {
    TEST_ASSERT_TRUE(meters_to_ticks(-1.0 * PI * 2.0 * 2.0 * WHEEL_RADIUS) == -TICKS_PER_ROTATION);
}

void test_conversions() {
    TEST_ASSERT_TRUE(meters_to_ticks(ticks_to_meters(TICKS_PER_ROTATION)) == TICKS_PER_ROTATION);
}

// A pure +y body velocity should drive wheel pairs (0,3) and (1,2) as mirrored
// opposites of each other, since the robot is symmetric front-to-back and
// left-to-right for this axis.
void test_body_to_wheels_up_and_down() {
    MotionControl motion_control = MotionControl();

    Vector3f up(0.0f, 1.0f, 0.0f);
    Vector4i wheels_up = motion_control.body_to_wheels(up);

    TEST_ASSERT_INT32_WITHIN(1, -wheels_up(3), wheels_up(0));
    TEST_ASSERT_INT32_WITHIN(1, -wheels_up(2), wheels_up(1));

    Vector3f down(0.0f, -1.0f, 0.0f);
    Vector4i wheels_down = motion_control.body_to_wheels(down);

    TEST_ASSERT_INT32_WITHIN(1, -wheels_down(3), wheels_down(0));
    TEST_ASSERT_INT32_WITHIN(1, -wheels_down(2), wheels_down(1));

    // And reversing the body direction should reverse every wheel
    TEST_ASSERT_INT32_WITHIN(1, -wheels_up(0), wheels_down(0));
    TEST_ASSERT_INT32_WITHIN(1, -wheels_up(1), wheels_down(1));
    TEST_ASSERT_INT32_WITHIN(1, -wheels_up(2), wheels_down(2));
    TEST_ASSERT_INT32_WITHIN(1, -wheels_up(3), wheels_down(3));
}

// A pure x body velocity should drive wheel pairs (0,3) and (1,2) as matching
// (not mirrored) since left/right strafing is symmetric front-to-back.
void test_body_to_wheels_left_and_right() {
    MotionControl motion_control = MotionControl();

    Vector3f left(-1.0f, 0.0f, 0.0f);
    Vector4i wheels_left = motion_control.body_to_wheels(left);

    TEST_ASSERT_INT32_WITHIN(1, wheels_left(3), wheels_left(0));
    TEST_ASSERT_INT32_WITHIN(1, wheels_left(2), wheels_left(1));

    Vector3f right(1.0f, 0.0f, 0.0f);
    Vector4i wheels_right = motion_control.body_to_wheels(right);

    TEST_ASSERT_INT32_WITHIN(1, wheels_right(3), wheels_right(0));
    TEST_ASSERT_INT32_WITHIN(1, wheels_right(2), wheels_right(1));
}

// A pure spin (w only) should drive all four wheels at equal magnitude.
void test_body_to_wheels_spin() {
    MotionControl motion_control = MotionControl();

    Vector3f spin(0.0f, 0.0f, 1.0f);
    Vector4i wheels = motion_control.body_to_wheels(spin);

    TEST_ASSERT_INT32_WITHIN(1, wheels(0), wheels(1));
    TEST_ASSERT_INT32_WITHIN(1, wheels(1), wheels(2));
    TEST_ASSERT_INT32_WITHIN(1, wheels(2), wheels(3));

    Vector3f no_spin(0.0f, 0.0f, 0.0f);
    Vector4i wheels_zero = motion_control.body_to_wheels(no_spin);
    TEST_ASSERT_EQUAL_INT32(0, wheels_zero(0));
    TEST_ASSERT_EQUAL_INT32(0, wheels_zero(1));
    TEST_ASSERT_EQUAL_INT32(0, wheels_zero(2));
    TEST_ASSERT_EQUAL_INT32(0, wheels_zero(3));
}

// control_update should short-circuit to a hard zero for a zero target,
// regardless of sensor input
void test_control_update_zero_target_is_zero() {
    MotionControl motion_control = MotionControl();

    Vector3f imu(2.0f, -1.5f, 0.3f);
    Vector4i encoders(500, -500, 500, -500);
    Vector3f zero_target = Vector3f::Zero();

    Vector4i result = motion_control.control_update(imu, encoders, zero_target, 16666);

    TEST_ASSERT_TRUE(result == Vector4i::Zero());
}

int main( int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_bot_to_wheel);
    RUN_TEST(test_wheel_to_bot);
    RUN_TEST(test_ticks_to_meters);
    RUN_TEST(test_ticks_to_meters_zero);
    RUN_TEST(test_ticks_to_meters_negative);
    RUN_TEST(test_meters_to_ticks);
    RUN_TEST(test_meters_to_ticks_negative);
    RUN_TEST(test_conversions);
    RUN_TEST(test_body_to_wheels_up_and_down);
    RUN_TEST(test_body_to_wheels_left_and_right);
    RUN_TEST(test_body_to_wheels_spin);
    RUN_TEST(test_control_update_zero_target_is_zero);

    UNITY_END();
}