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

    // Team circle
    u8g2->drawStr(65, 48, (team == Team::Blue ? "B" : "Y"));
    u8g2->drawCircle(67, 43, 8, U8G2_DRAW_ALL);

    // ID circles
    u8g2->drawStr(32, 30, id_colors[id][0]);
    u8g2->drawCircle(35, 26, 8, U8G2_DRAW_ALL);
    u8g2->drawStr(96, 30, id_colors[id][1]);
    u8g2->drawCircle(99, 26, 8, U8G2_DRAW_ALL);
    u8g2->drawStr(40, 57, id_colors[id][2]);
    u8g2->drawCircle(43, 53, 8, U8G2_DRAW_ALL);
    u8g2->drawStr(88, 57, id_colors[id][3]);
    u8g2->drawCircle(91, 53, 8, U8G2_DRAW_ALL);
}