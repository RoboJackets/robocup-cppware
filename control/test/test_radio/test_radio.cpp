#include <unity.h>
#include <Arduino.h>
#include <ArduinoEigen.h>
using namespace Eigen;
 
#include "radio.hpp"
 
void setUp(void) {}
void tearDown(void) {}
 
bool float_compare(float a, float b) {
    return (int(a * 100000000) == int(b * 100000000));
}

void test_control_message_unpack() {
    uint8_t data[CONTROL_MESSAGE_SIZE] = {0};
 
    // Byte 0: team=Yellow (bit7), robot_id=9 (bits6-3), shoot_mode=Chip (bit2),
    // trigger_mode=Immediate (bits1-0 == 0b01)
    data[0] = (1 << 7) | (9 << 3) | (1 << 2) | 0b01;
 
    // body_x = 1000 (0x03E8), little-endian
    int16_t body_x = 1000;
    data[1] = body_x & 0xFF;
    data[2] = (body_x >> 8) & 0xFF;
 
    // body_y = -500 (0xFE0C two's complement), little-endian
    int16_t body_y = -500;
    data[3] = body_y & 0xFF;
    data[4] = (body_y >> 8) & 0xFF;
 
    // body_w = 250, little-endian
    int16_t body_w = 250;
    data[5] = body_w & 0xFF;
    data[6] = (body_w >> 8) & 0xFF;
 
    // dribbler_speed = -10 (as raw byte, interpreted as int8_t)
    data[7] = (uint8_t)(-10);
 
    // kick_strength = 12
    data[8] = 12;
 
    // role = 2 (bits7-6), mode = 37 (bits5-0)
    data[9] = (2 << 6) | 37;
 
    ControlMessage msg;
    msg.unpack(data);
 
    TEST_ASSERT_EQUAL(Team::Yellow, msg.team);
    TEST_ASSERT_EQUAL_UINT8(9, msg.robot_id);
    TEST_ASSERT_EQUAL(ShootMode::Chip, msg.shoot_mode);
    TEST_ASSERT_EQUAL(TriggerMode::Immediate, msg.trigger_mode);
    TEST_ASSERT_EQUAL_INT16(1000, msg.body_x);
    TEST_ASSERT_EQUAL_INT16(-500, msg.body_y);
    TEST_ASSERT_EQUAL_INT16(250, msg.body_w);
    TEST_ASSERT_EQUAL_INT8(-10, msg.dribbler_speed);
    TEST_ASSERT_EQUAL_UINT8(12, msg.kick_strength);
    TEST_ASSERT_EQUAL_UINT8(2, msg.role);
    TEST_ASSERT_EQUAL_UINT8(37, msg.mode);
}
 
// The two trigger-mode bit patterns not covered above (Breakbeam and Disabled),
// since unpack() decides between three outcomes via a 2-bit switch.
void test_control_message_unpack_trigger_modes() {
    uint8_t data[CONTROL_MESSAGE_SIZE] = {0};
 
    data[0] = 0b10; // Breakbeam
    ControlMessage msg_breakbeam;
    msg_breakbeam.unpack(data);
    TEST_ASSERT_EQUAL(TriggerMode::Breakbeam, msg_breakbeam.trigger_mode);
 
    data[0] = 0b00; // Disabled (also covers the "default" case, e.g. 0b11)
    ControlMessage msg_disabled;
    msg_disabled.unpack(data);
    TEST_ASSERT_EQUAL(TriggerMode::Disabled, msg_disabled.trigger_mode);
 
    data[0] = 0b11; // Disabled via default branch
    ControlMessage msg_disabled_2;
    msg_disabled_2.unpack(data);
    TEST_ASSERT_EQUAL(TriggerMode::Disabled, msg_disabled_2.trigger_mode);
}
 
// get_velocity() must divide the raw scaled ints back down by VELOCITY_SCALE_FACTOR
void test_control_message_get_velocity() {
    ControlMessage msg;
    msg.body_x = 1000;
    msg.body_y = -500;
    msg.body_w = 250;
 
    Vector3f velocity = msg.get_velocity();
 
    TEST_ASSERT_TRUE(float_compare(velocity(0), 1.0f));
    TEST_ASSERT_TRUE(float_compare(velocity(1), -0.5f));
    TEST_ASSERT_TRUE(float_compare(velocity(2), 0.25f));
}
 
void test_control_message_get_velocity_zero() {
    ControlMessage msg;
    Vector3f velocity = msg.get_velocity();
 
    TEST_ASSERT_TRUE(float_compare(velocity(0), 0.0f));
    TEST_ASSERT_TRUE(float_compare(velocity(1), 0.0f));
    TEST_ASSERT_TRUE(float_compare(velocity(2), 0.0f));
}

void test_robot_status_message_pack_matches_documented_layout() {
    RobotStatusMessage status;
    status.team = Team::Yellow;
    status.robot_id = 5;
    status.ball_sense_status = true;
    status.kick_status = true;
    status.kick_healthy = true;
    status.battery_percent = 77;
    status.motor_errors = 0b10101;
    status.fpga_status = true;
 
    uint8_t pkg[ROBOT_STATUS_SIZE] = {0};
    status.pack(pkg);
 
    uint8_t expected_byte0 = (1 << 7)               // team = Yellow
                            | (5 << 3)               // robot_id = 5
                            | (1 << 2)                // ball_sense_status
                            | (1 << 1)                // kick_status
                            | 1;                       // kick_healthy
 
    uint8_t expected_byte2 = (0b10101 << 3)          // motor_errors
                            | (1 << 2);                // fpga_status
 
    TEST_ASSERT_EQUAL_UINT8(expected_byte0, pkg[0]);
    TEST_ASSERT_EQUAL_UINT8(77, pkg[1]);
    TEST_ASSERT_EQUAL_UINT8(expected_byte2, pkg[2]);
}
 
void test_robot_status_message_pack_does_not_depend_on_prezeroed_buffer() {
    RobotStatusMessage status; // all defaults: team=Blue, robot_id=0, everything false/0
 
    uint8_t pkg[ROBOT_STATUS_SIZE];
    memset(pkg, 0xFF, sizeof(pkg)); // simulate "garbage" stack memory
 
    status.pack(pkg);
 
    TEST_ASSERT_EQUAL_UINT8(0, pkg[0]);
    TEST_ASSERT_EQUAL_UINT8(0, pkg[1]);
    TEST_ASSERT_EQUAL_UINT8(0, pkg[2]);
}
 
void test_acks_to_percent_all_good() {
    bool acks[100];
    memset(acks, true, sizeof(acks));
    TEST_ASSERT_EQUAL_UINT8(100, acks_to_percent(acks));
}
 
void test_acks_to_percent_all_bad() {
    bool acks[100];
    memset(acks, false, sizeof(acks));
    TEST_ASSERT_EQUAL_UINT8(0, acks_to_percent(acks));
}
 
void test_acks_to_percent_partial() {
    bool acks[100];
    memset(acks, false, sizeof(acks));
    for (size_t i = 0; i < 42; i++) acks[i] = true;
    TEST_ASSERT_EQUAL_UINT8(42, acks_to_percent(acks));
}
 
int main(int argc, char **argv) {
    UNITY_BEGIN();
 
    RUN_TEST(test_control_message_unpack);
    RUN_TEST(test_control_message_unpack_trigger_modes);
    RUN_TEST(test_control_message_get_velocity);
    RUN_TEST(test_control_message_get_velocity_zero);
    RUN_TEST(test_robot_status_message_pack_matches_documented_layout);
    RUN_TEST(test_robot_status_message_pack_does_not_depend_on_prezeroed_buffer);
    RUN_TEST(test_acks_to_percent_all_good);
    RUN_TEST(test_acks_to_percent_all_bad);
    RUN_TEST(test_acks_to_percent_partial);
 
    UNITY_END();
}
