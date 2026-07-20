#include "screen.hpp"


void screen_defaults(U8G2* u8g2) {
    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->setDrawColor(1);
    u8g2->setFontMode(1);
}

void draw_startup(U8G2* u8g2, uint8_t dots) {
    screen_defaults(u8g2);
    u8g2->drawStr(0, 11, "Awaiting Serial");
    for (int i = 0; i < dots; i++) {
        u8g2->drawStr(6 * 15 + 6 * i, 11, ".");
    }
    u8g2->drawXBMP(0, LAST_YELLOW_Y + 1, 27, 47, robobuzz);
    u8g2->drawXBMP(28, LAST_YELLOW_Y + 24, 101, 7, logo_text);
}

void draw_battery(U8G2* u8g2, uint32_t x, uint32_t y, uint8_t percent) {
    screen_defaults(u8g2);
    if (percent > 100) {
        u8g2->drawStr(x, y + 6, "ERROR");
        return;
    }
    uint8_t level = 8 * percent / 100;
    u8g2->drawXBMP(x, y, 12, 6, battery_icon[level]);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    u8g2->drawStr(x + 13, y + 6, buf);
}

void draw_header(U8G2* u8g2, RobotStatusMessage status) {
    screen_defaults(u8g2);
    draw_battery(u8g2, 0, 4, status.battery_percent);
    u8g2->drawLine(0, LAST_YELLOW_Y, 128, LAST_YELLOW_Y);
    char buf[32];
    snprintf(buf, sizeof(buf), "Team: %s ID: %d", (status.team == Team::Blue ? "B" : "Y"), status.robot_id);
    u8g2->drawStr(48, 10, buf);
}

void draw_info(U8G2* u8g2, RobotStatusMessage status, bool radio, uint8_t kicker_voltage) {
    screen_defaults(u8g2);
    char buf[32];
    snprintf(buf, sizeof(buf), "Radio: %s", (radio ? "GOOD" : "DEAD"));
    u8g2->drawStr(0, LAST_YELLOW_Y + 8, buf);
    snprintf(buf, sizeof(buf), "Kicker: %s V: %d", (status.kick_healthy ? "GOOD" : "DEAD"), kicker_voltage);
    u8g2->drawStr(0, LAST_YELLOW_Y + 16, buf);
    snprintf(buf, sizeof(buf), "Kicking: %s ", (status.kick_status ? "RDY" : "NOT"));
    u8g2->drawStr(0, LAST_YELLOW_Y + 24, buf);
}

void draw_colors(U8G2* u8g2, Team team, uint8_t id) {
    screen_defaults(u8g2);
    u8g2->setFont(u8g2_font_10x20_tr);

    // Team circle
    u8g2->drawStr(65, 48, (team == Team::Blue ? "B" : "Y"));
    // ID circles
    u8g2->drawStr(32, 30, id_colors[id][0]);
    u8g2->drawStr(96, 30, id_colors[id][1]);
    u8g2->drawStr(40, 57, id_colors[id][2]);
    u8g2->drawStr(88, 57, id_colors[id][3]);
}

void send_buffer_fast(U8G2* u8g2) {
    uint8_t *buf = u8g2->getBufferPtr();

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