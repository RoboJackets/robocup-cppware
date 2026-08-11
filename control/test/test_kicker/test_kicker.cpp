#include <unity.h>
#include <Arduino.h>
#include <SPI.h>

#include "kicker.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_kicker_command_pack_defaults() {
    KickerCommand cmd; // Kick, Disabled, strength=0, charge_allowed=false
    uint16_t expected = ShootMode::Kick | TriggerMode::Disabled;
    TEST_ASSERT_EQUAL_UINT16(expected, cmd.pack());
}

void test_kicker_command_pack_full() {
    KickerCommand cmd;
    cmd.shoot_mode = ShootMode::Chip;
    cmd.trigger_mode = TriggerMode::Immediate;
    cmd.kick_strength = 9;
    cmd.charge_allowed = true;

    uint16_t expected = ShootMode::Chip | TriggerMode::Immediate | (1 << 4) | (9 & 0xF);
    TEST_ASSERT_EQUAL_UINT16(expected, cmd.pack());
}

void test_kicker_command_pack_masks_kick_strength() {
    KickerCommand cmd;
    cmd.shoot_mode = ShootMode::Kick;
    cmd.trigger_mode = TriggerMode::Disabled;
    cmd.kick_strength = 0xFF; // deliberately out of the documented 0-15 range
    cmd.charge_allowed = false;

    uint16_t result = cmd.pack();
    uint16_t expected = ShootMode::Kick | TriggerMode::Disabled | (0xFF & 0xF);
    TEST_ASSERT_EQUAL_UINT16(expected, result);
}

Kicker make_test_kicker() {
    return Kicker(SPI1, SPISettings(2000000, MSBFIRST, SPI_MODE3), 0, 0);
}

void test_kicker_update_state_unhealthy() {
    Kicker kicker = make_test_kicker();

    uint8_t voltage_half = 100;      // bits 0-6
    bool ball_sensed = true;         // bit 7
    uint8_t error_code = KickerError::BreakbeamBlockage; // bits 8-15

    uint16_t raw = voltage_half | (ball_sensed << 7) | (error_code << 8);
    kicker.update_state(raw);

    TEST_ASSERT_EQUAL_UINT8(200, kicker.current_voltage); // voltage_half * 2
    TEST_ASSERT_TRUE(kicker.ball_sensed);
    TEST_ASSERT_EQUAL(KickerError::BreakbeamBlockage, kicker.error);
    TEST_ASSERT_FALSE(kicker.healthy);
}

void test_kicker_update_state_healthy() {
    Kicker kicker = make_test_kicker();

    uint8_t voltage_half = 63;
    bool ball_sensed = false;
    uint8_t error_code = KickerError::None;

    uint16_t raw = voltage_half | (ball_sensed << 7) | (error_code << 8);
    kicker.update_state(raw);

    TEST_ASSERT_EQUAL_UINT8(126, kicker.current_voltage);
    TEST_ASSERT_FALSE(kicker.ball_sensed);
    TEST_ASSERT_EQUAL(KickerError::None, kicker.error);
    TEST_ASSERT_TRUE(kicker.healthy);
}

void test_kicker_update_state_voltage_upper_bound() {
    Kicker kicker = make_test_kicker();

    // voltage field is 7 bits (0-127); make sure the ball-sensed bit (bit 7)
    // never leaks into the voltage calculation.
    uint16_t raw = 0x7F | (1 << 7); // max voltage field + ball sensed set
    kicker.update_state(raw);

    TEST_ASSERT_EQUAL_UINT8(254, kicker.current_voltage); // 127 * 2
    TEST_ASSERT_TRUE(kicker.ball_sensed);
}

void test_triggermode_to_str() {
    TEST_ASSERT_EQUAL_STRING("Immediate", triggermode_to_str(TriggerMode::Immediate).c_str());
    TEST_ASSERT_EQUAL_STRING("Breakbeam", triggermode_to_str(TriggerMode::Breakbeam).c_str());
    TEST_ASSERT_EQUAL_STRING("Disabled", triggermode_to_str(TriggerMode::Disabled).c_str());
}

void test_shootmode_to_str() {
    TEST_ASSERT_EQUAL_STRING("Kick", shootmode_to_str(ShootMode::Kick).c_str());
    TEST_ASSERT_EQUAL_STRING("Chip", shootmode_to_str(ShootMode::Chip).c_str());
}

void test_kicker_error_to_str() {
    TEST_ASSERT_EQUAL_STRING("None", kicker_error_to_str(KickerError::None));
    TEST_ASSERT_EQUAL_STRING("BreakbeamBlockage", kicker_error_to_str(KickerError::BreakbeamBlockage));
    TEST_ASSERT_EQUAL_STRING("MAJOR OVER VOLTAGE", kicker_error_to_str(KickerError::MajorOverVoltage));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_kicker_command_pack_defaults);
    RUN_TEST(test_kicker_command_pack_full);
    RUN_TEST(test_kicker_command_pack_masks_kick_strength);
    RUN_TEST(test_kicker_update_state_unhealthy);
    RUN_TEST(test_kicker_update_state_healthy);
    RUN_TEST(test_kicker_update_state_voltage_upper_bound);
    RUN_TEST(test_triggermode_to_str);
    RUN_TEST(test_shootmode_to_str);
    RUN_TEST(test_kicker_error_to_str);

    UNITY_END();
}