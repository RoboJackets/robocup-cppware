/*
A basic display driver to wrap the u8g2 driver in simplified functions
This can be considered mess and should be taken with a grain of salt
*/

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "radio.hpp"
#include "types.hpp"

// Y value of the last yellow pixel, +1 is the first blue pixel
#define LAST_YELLOW_Y 15
// Display Width
#define DISPLAY_WIDTH 128
// Display Height
#define DISPLAY_HEIGHT 64

// battery icons, 12x6px
static const unsigned char battery_icon[9][12] U8X8_PROGMEM = {
    {0xff, 0x08, 0x81, 0x0b, 0x01, 0x0a, 0x01, 0x0a, 0x81, 0x03, 0xff, 0x08},
    {0xff, 0x00, 0x81, 0x03, 0x01, 0x02, 0x01, 0x02, 0x81, 0x03, 0xff, 0x00},
    {0xff, 0x00, 0x83, 0x03, 0x03, 0x02, 0x03, 0x02, 0x83, 0x03, 0xff, 0x00},
    {0xff, 0x00, 0x87, 0x03, 0x07, 0x02, 0x07, 0x02, 0x87, 0x03, 0xff, 0x00},
    {0xff, 0x00, 0x8f, 0x03, 0x0f, 0x02, 0x0f, 0x02, 0x8f, 0x03, 0xff, 0x00},
    {0xff, 0x00, 0x9f, 0x03, 0x1f, 0x02, 0x1f, 0x02, 0x9f, 0x03, 0xff, 0x00},
    {0xff, 0x00, 0xbf, 0x03, 0x3f, 0x02, 0x3f, 0x02, 0xbf, 0x03, 0xff, 0x00},
    {0xff, 0x00, 0xff, 0x03, 0x7f, 0x02, 0x7f, 0x02, 0xff, 0x03, 0xff, 0x00},
    {0xff, 0x00, 0xff, 0x03, 0xff, 0x02, 0xff, 0x02, 0xff, 0x03, 0xff, 0x00}
};

// robobuzz, 27x47px
static const unsigned char robobuzz [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 
	0x20, 0x01, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
	0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x1f, 0x00, 0x80, 0x1f, 0x1f, 0x00, 
	0xc0, 0x38, 0x3f, 0x00, 0x60, 0x70, 0x3f, 0x00, 0x30, 0x70, 0x3f, 0x00, 0x38, 0x70, 0x1f, 0x01, 
	0x1c, 0x78, 0xff, 0x03, 0x1c, 0x7c, 0xff, 0x03, 0xfe, 0xbf, 0xff, 0x03, 0xfe, 0x9f, 0xff, 0x03, 
	0xff, 0x8f, 0xdf, 0x03, 0xff, 0x8f, 0x81, 0x03, 0xff, 0x17, 0xc1, 0x03, 0x3e, 0x7e, 0x60, 0x02, 
	0x1e, 0x7c, 0xc0, 0x03, 0x08, 0x38, 0x63, 0x00, 0xc8, 0xf0, 0x4f, 0x02, 0xf8, 0x61, 0x1e, 0x01, 
	0x58, 0xe1, 0x18, 0x00, 0xc0, 0xf0, 0x1f, 0x00, 0xc0, 0xe0, 0x1f, 0x00, 0x00, 0xe0, 0x3e, 0x00, 
	0x00, 0x70, 0x13, 0x00, 0x00, 0x78, 0x13, 0x00, 0x00, 0xfc, 0x0b, 0x00, 0x00, 0xfc, 0x0f, 0x00, 
	0x00, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0xf8, 0x0d, 0x00, 0x00, 0xf8, 0x01, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// robojackets, 101x7px
static const unsigned char logo_text [] PROGMEM = {
	0xff, 0x01, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0xf9, 0xf7, 
	0xc7, 0x3f, 0x30, 0x18, 0xfe, 0x84, 0xf1, 0xf7, 0xdf, 0x1f, 0x83, 0x19, 0x34, 0x7c, 0x30, 0x30, 
	0x3c, 0x86, 0xfc, 0x30, 0x80, 0x63, 0x00, 0xff, 0x09, 0xf4, 0x67, 0xb0, 0x31, 0x66, 0x06, 0x7c, 
	0xf0, 0x80, 0xc1, 0x1f, 0xe3, 0x08, 0x34, 0x7c, 0xb0, 0x31, 0xff, 0x86, 0x6d, 0x30, 0x80, 0x01, 
	0x18, 0xc3, 0xf9, 0xf7, 0xff, 0xbf, 0x3f, 0x80, 0xfe, 0x04, 0xf1, 0x87, 0xe1, 0x1f, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Window selector
enum Window {
    Info = 0,
    Colors = 1,

};

// Display wrapper class
class Display {
public:
    /**
     * Initializes I2C display
     * 
     * @param frequency I2C Wire frequency
     */
    void begin(uint32_t frequency);

    /**
     * Clears current screen buffer
     * 
     * @note Call this each time before starting a new screen update to prevent data mashing
     */
    void clear_buffer();

    /**
     * Sends current buffer to display
     * 
     * @note This gives an additional decrease of 7ms of write time compared to the base
     * u8g2 sendBuffer function.
     * 
     * @warning This is technically gpt code however I have not run into any issues with it.
     */
    void send_buffer();

    /**
     * Applies default write settings to display
     * 
     * @note This is called at the start of all draw functions to prevent unknown settings
     */
    void defaults();

    /**
     * A simple hello world test to ensure screen is working
     * 
     * @note Clears and sends buffer on its own
     */
    void test_display();

    /**
     * Updates all stored info for screen display
     * 
     * @param status Robot status
     * @param radio_status True if radio is connected
     * @param kicker_voltage Current kicker voltage
     * @param ack_percent Current radio acknowledgment percent 0-100
     */
    void update_info(RobotStatusMessage status, bool radio_status, uint8_t kicker_voltage, uint8_t ack_percent);

    /**
     * Draws the current window defined by `current_window`  to buffer
     * @see
     * - `window_select()`
     * - `next_window()`
     * 
     * @note Windows are defined as the lower blue pixels so should only write below `LOWEST_YELLOW_Y`
     */
    void draw_window();

    /**
     * Draws the current header to buffer
     * 
     * @note Headers are defined as the upper yellow pixels so should only write at/above `LOWEST_YELLOW_Y`
     */
    void draw_header();

    /**
     * Sets current window
     * 
     * @param window Window to be drawn by `draw_window()`
     */
    void window_select(Window window);

    /**
     * Increments the current window
     * 
     * @note Order is determined by the `Window` enum
     */
    void next_window();

    /**
     * Draws the "info" window to buffer
     */
    void draw_info();

    /**
     * Draws the "colors" window to buffer
     */
    void draw_colors();

    /**
     * Draws the startup screen to buffer
     * 
     * @param dots number of dots after text for animation purposes
     * 
     * @note Not a header or window, uses full display
     */
    void draw_startup(uint8_t dots);

    /**
     * Draws a battery icon with a set fullness
     * 
     * @param x Leftmost pixel
     * @param y Topmost pixel
     * @param percent Fullness of battery 0-100 (Values outside this display as "ERROR")
     */
    void draw_battery(uint8_t x, uint8_t y, uint8_t percent);

    /**
     * Draws error screen with flashing indicator for attention
     * 
     * @param error current error to change what text is displayed
     * 
     * @note Not a header or window, uses full display
     */
    void draw_error(RobotError error);

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2{U8G2_R0, U8X8_PIN_NONE};
    // Current window for draw_window()
    Window _current_window = Window::Info;
    // True if good radio
    bool _radio_status = false;
    // True if good kicker
    bool _kicker_status = false;
    // True if kick queued
    bool _kicking = false;
    // Current kicker voltage
    uint8_t _kicker_voltage = 0;
    // Current battery percent 0-100
    uint8_t _battery_percent = 0;
    // Current radio success rate
    uint8_t _ack_percent;
    // Robot team
    Team _team = Team::Blue;
    // Robot id
    uint8_t _id = 0;

    // Colors for team order goes FL, FR, BL, BR
    static constexpr const char* id_colors[16][4] = {
        {"P", "P", "G", "P"},
        {"G", "P", "G", "P"},
        {"G", "G", "G", "P"},
        {"P", "G", "G", "P"},
        {"P", "P", "P", "G"},
        {"G", "P", "P", "G"},
        {"G", "G", "P", "G"},
        {"P", "G", "P", "G"},
        {"G", "G", "G", "G"},
        {"P", "P", "P", "P"},
        {"P", "P", "G", "G"},
        {"G", "G", "P", "P"},
        {"G", "P", "G", "G"},
        {"G", "P", "P", "P"},
        {"P", "G", "G", "G"},
        {"P", "G", "P", "P"},
    };
};

/**
 * A simple circular mod allowing for increment and decrement
 * 
 * @param var Value to mod
 * @param increment Value to increment/decrement `var` by
 * @param max Maximum value of `var`
 */
inline int16_t circular_mod(int16_t var, int16_t increment, int16_t max) {
    return ((var + increment) % max + max) % max;
}