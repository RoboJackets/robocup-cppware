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

void test_meters_to_ticks() {
    TEST_ASSERT_TRUE(meters_to_ticks(1.0 * PI * 2.0 * 2.0 * WHEEL_RADIUS) == TICKS_PER_ROTATION);
}

void test_conversions() {
    TEST_ASSERT_TRUE(meters_to_ticks(ticks_to_meters(TICKS_PER_ROTATION)) == TICKS_PER_ROTATION);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_bot_to_wheel);
    RUN_TEST(test_wheel_to_bot);
    RUN_TEST(test_ticks_to_meters);
    RUN_TEST(test_meters_to_ticks);
    RUN_TEST(test_conversions);

    UNITY_END();
}