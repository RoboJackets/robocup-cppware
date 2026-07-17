#include <unity.h>
#include <Arduino.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_addition() {
    uint8_t a = 1;
    uint8_t b = 1;
    TEST_ASSERT_TRUE(a + b == 2);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_addition);

    UNITY_END();
}