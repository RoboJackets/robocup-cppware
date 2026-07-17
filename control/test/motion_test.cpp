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

void test_bot_to_wheels() {
    MotionControl motion_control = MotionControl();

    Matrix<float, 4, 3> expected_matrix {
        {0.50000006f, 0.8660254f, -0.0779815f,},
        {-0.7071067f, 0.7071068f, -0.0779815f,},
        {-0.7071069f, -0.70710665f, -0.0779815f,},
        {0.5f, -0.8660254f, -0.0779815f,}
    };


    TEST_ASSERT_TRUE(motion_control.bot_to_wheel == expected_matrix);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_bot_to_wheels);

    UNITY_END();
}