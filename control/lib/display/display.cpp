#include "display.hpp"

void Display::begin(uint32_t frequency) {
    u8g2.begin();
    u8g2.setBusClock(frequency);
    Wire.setClock(frequency);
    Display::defaults();
}

void Display::defaults() {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setDrawColor(1);
    u8g2.setFontMode(1);
}

void Display::clear_buffer() {
    u8g2.clearBuffer();
}

void Display::send_buffer() {
    uint8_t *buf = u8g2.getBufferPtr();

    for (uint8_t page = 0; page < 8; page++) {
        // Tell screen where to write
        Wire.beginTransmission(0x3C); // Address
        Wire.write(0x00); // Following bytes are commands
        Wire.write(0xB0 | page); // Set display page
        Wire.write(0x00); // Column address lower nibble = 0
        Wire.write(0x10); // Column address upper nibble = 0
        Wire.endTransmission();

        // Send actual data
        Wire.beginTransmission(0x3C);
        Wire.write(0x40); // Next bytes are display data
        Wire.write(&buf[page * 128], 128);
        Wire.endTransmission();
    }
}

void Display::test_display() {
    Display::clear_buffer();
    Display::defaults();
    u8g2.drawStr(0, 11, "Hello World!");
    Display::send_buffer();
}

void Display::update_info(RobotStatusMessage status, bool _radio_status, uint8_t _kicker_voltage, uint8_t _ack_percent) {
    radio_status = _radio_status;
    kicker_voltage = _kicker_voltage;
    ack_percent = _ack_percent;
    battery_percent = status.battery_percent;
    kicker_status = status.kick_healthy;
    kicking = status.kick_status;
    team = status.team;
    id = status.robot_id;
}

void Display::draw_header() {
    Display::defaults();
    Display::draw_battery(0, 4, battery_percent);
    char buf[32];
    snprintf(buf, sizeof(buf), "Team: %s ID: %d", (team == Team::Blue ? "B" : "Y"), id);
    u8g2.drawStr(48, 10, buf);
    u8g2.drawLine(0, LAST_YELLOW_Y, DISPLAY_WIDTH, LAST_YELLOW_Y);
}

void Display::draw_battery(uint8_t x, uint8_t y, uint8_t percent) {
    Display::defaults();
    if (percent > 100) {
        u8g2.drawStr(x, y + 6, "ERROR");
        return;
    }
    uint8_t level = 8 * percent / 100;
    u8g2.drawXBMP(x, y, 12, 6, battery_icon[level]);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    u8g2.drawStr(x + 13, y + 6, buf);
}

void Display::draw_window() {
    switch (current_window) {
        case Colors:
            Display::draw_colors();
            break;
        case Info:
        default:
            Display::draw_info();
    }
}

void Display::draw_info() {
    Display::defaults();
    char buf[32];
    snprintf(buf, sizeof(buf), "Radio: %s SR: %d%%", (radio_status ? "GOOD" : "DEAD"), ack_percent);
    u8g2.drawStr(0, LAST_YELLOW_Y + 8, buf);
    snprintf(buf, sizeof(buf), "Kicker: %s V: %dv", (kicker_status ? "GOOD" : "DEAD"), kicker_voltage);
    u8g2.drawStr(0, LAST_YELLOW_Y + 16, buf);
    snprintf(buf, sizeof(buf), "Kicking: %s ", (kicking ? "RDY" : "NOT"));
    u8g2.drawStr(0, LAST_YELLOW_Y + 24, buf);
}

void Display::draw_colors() {
    Display::defaults();
    u8g2.setFont(u8g2_font_10x20_tr);

    // Team circle
    u8g2.drawStr(65, 48, (team == Team::Blue ? "B" : "Y"));
    // ID circles
    u8g2.drawStr(32, 30, id_colors[id][0]);
    u8g2.drawStr(96, 30, id_colors[id][1]);
    u8g2.drawStr(40, 57, id_colors[id][2]);
    u8g2.drawStr(88, 57, id_colors[id][3]);
}

void Display::draw_startup(uint8_t dots) {
    Display::defaults();
    u8g2.drawStr(0, 11, "Awaiting Serial");
    for (int i = 0; i < dots; i++) {
        u8g2.drawStr(6 * 15 + 6 * i, 11, ".");
    }
    u8g2.drawXBMP(0, LAST_YELLOW_Y + 1, 27, 47, robobuzz);
    u8g2.drawXBMP(28, LAST_YELLOW_Y + 24, 101, 7, logo_text);
}
 
void Display::window_select(Window window) {
    current_window = window;
}

void Display::next_window() {
    current_window = static_cast<Window>(circular_mod((int16_t)current_window, 1, 2));
}