#include "display.hpp"

void Display::begin(uint32_t frequency) {
    _u8g2.begin();
    _u8g2.setBusClock(frequency);
    Wire.setClock(frequency);
    Display::defaults();
}

void Display::defaults() {
    _u8g2.setFont(u8g2_font_6x10_tr);
    _u8g2.setDrawColor(1);
    _u8g2.setFontMode(1);
}

void Display::clear_buffer() {
    _u8g2.clearBuffer();
}

void Display::send_buffer() {
    uint8_t *buf = _u8g2.getBufferPtr();

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
    _u8g2.drawStr(0, 11, "Hello World!");
    Display::send_buffer();
}

void Display::update_info(RobotStatusMessage status, bool radio_status, uint8_t kicker_voltage, uint8_t ack_percent) {
    _radio_status = radio_status;
    _kicker_voltage = kicker_voltage;
    _ack_percent = ack_percent;
    _battery_percent = status.battery_percent;
    _kicker_status = status.kick_healthy;
    _kicking = status.kick_status;
    _team = status.team;
    _id = status.robot_id;
}

void Display::draw_header() {
    Display::defaults();
    Display::draw_battery(0, 4, _battery_percent);
    char buf[32];
    snprintf(buf, sizeof(buf), "Team: %s ID: %d", (_team == Team::Blue ? "B" : "Y"), _id);
    _u8g2.drawStr(48, 10, buf);
    _u8g2.drawLine(0, LAST_YELLOW_Y, DISPLAY_WIDTH, LAST_YELLOW_Y);
}

void Display::draw_battery(uint8_t x, uint8_t y, uint8_t percent) {
    Display::defaults();
    if (percent > 100) {
        _u8g2.drawStr(x, y + 6, "ERROR");
        return;
    }
    uint8_t level = 8 * percent / 100;
    _u8g2.drawXBMP(x, y, 12, 6, battery_icon[level]);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    _u8g2.drawStr(x + 13, y + 6, buf);
}

void Display::draw_window() {
    switch (_current_window) {
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
    snprintf(buf, sizeof(buf), "Radio: %s SR: %d%%", (_radio_status ? "GOOD" : "DEAD"), _ack_percent);
    _u8g2.drawStr(0, LAST_YELLOW_Y + 8, buf);
    snprintf(buf, sizeof(buf), "Kicker: %s V: %dv", (_kicker_status ? "GOOD" : "DEAD"), _kicker_voltage);
    _u8g2.drawStr(0, LAST_YELLOW_Y + 16, buf);
    snprintf(buf, sizeof(buf), "Kicking: %s ", (_kicking ? "RDY" : "NOT"));
    _u8g2.drawStr(0, LAST_YELLOW_Y + 24, buf);
    uint32_t now = millis();
    snprintf(buf, sizeof(buf), "Uptime: %ldh %ldm %lds", now / 3600000, (now % 3600000) / 60000, (now % 60000) / 1000);
    _u8g2.drawStr(0, LAST_YELLOW_Y + 32, buf);
}

void Display::draw_colors() {
    Display::defaults();
    _u8g2.setFont(u8g2_font_10x20_tr);

    // Team circle
    _u8g2.drawStr(65, 48, (_team == Team::Blue ? "B" : "Y"));
    // ID circles
    _u8g2.drawStr(32, 30, id_colors[_id][0]);
    _u8g2.drawStr(96, 30, id_colors[_id][1]);
    _u8g2.drawStr(40, 57, id_colors[_id][2]);
    _u8g2.drawStr(88, 57, id_colors[_id][3]);
}

void Display::draw_death() {
    Display::defaults();

    _u8g2.drawXBMP(0, LAST_YELLOW_Y + 1, 128, 49, death_window);
}

void Display::draw_startup(uint8_t dots) {
    Display::defaults();
    _u8g2.drawStr(0, 11, "Awaiting Serial");
    for (int i = 0; i < dots; i++) {
        _u8g2.drawStr(6 * 15 + 6 * i, 11, ".");
    }
    _u8g2.drawXBMP(0, LAST_YELLOW_Y + 1, 27, 47, robobuzz);
    _u8g2.drawXBMP(28, LAST_YELLOW_Y + 24, 101, 7, logo_text);
}
 
void Display::window_select(Window window) {
    _current_window = window;
}

void Display::next_window() {
    _current_window = static_cast<Window>(circular_mod((int16_t)_current_window, 1, 2));
}

static bool error_flash = false;
void Display::draw_error(RobotError error) {
    Display::defaults();
    _u8g2.setFont(u8g2_font_10x20_tr);

    _u8g2.drawStr((DISPLAY_WIDTH - 5 * 10) / 2, LAST_YELLOW_Y - 1, "ERROR");

    if (error_flash) {
        _u8g2.drawBox(0, 0, 30, 15);
    } else {
        _u8g2.drawBox(DISPLAY_WIDTH - 30, 0, 30, 15);
    }
    error_flash = !error_flash;
    
    
    
    switch (error) {
        case RobotError::RadioError:
            _u8g2.drawStr((DISPLAY_WIDTH - 5 * 10) / 2, LAST_YELLOW_Y + 16, "RADIO");
            _u8g2.drawStr((DISPLAY_WIDTH - 9 * 10) / 2, LAST_YELLOW_Y + 30, "INIT FAIL");
        break;
        case RobotError::UnrecoverableKicker:
        case RobotError::RecoverableKicker:
            _u8g2.drawStr((DISPLAY_WIDTH - 5 * 10) / 2, LAST_YELLOW_Y + 16, "KICKER");
            _u8g2.drawStr((DISPLAY_WIDTH - 5 * 10) / 2, LAST_YELLOW_Y + 30, "ERROR");
        break;
        case RobotError::BatteryUndervolt:
            _u8g2.drawStr((DISPLAY_WIDTH - 7 * 10) / 2, LAST_YELLOW_Y + 16, "BATTERY");
            _u8g2.drawStr((DISPLAY_WIDTH - 8 * 10) / 2, LAST_YELLOW_Y + 30, "DEPLETED");
            break;
        default:
            _u8g2.drawStr((DISPLAY_WIDTH - 6 * 10) / 2, LAST_YELLOW_Y + 16, "UNKOWN");
            _u8g2.drawStr((DISPLAY_WIDTH - 5 * 10) / 2, LAST_YELLOW_Y + 30, "ERROR");
        break;
    }
    
}