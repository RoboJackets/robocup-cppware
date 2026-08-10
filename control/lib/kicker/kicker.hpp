#pragma once

#include <Arduino.h>
#include <SPI.h>

// Type of kick to be performed by the kicker
enum ShootMode {
    Kick = 0,
    Chip = 1 << 7,
};

/**
 * Convert a `ShootMode` to a String
 * 
 * @param mode ShootMode to translate
 * 
 * @return String format of mode
 */
String shootmode_to_str(ShootMode mode);

// What trigger will activate the kicker: None, Breakbeam, or Immediate
enum TriggerMode {
    Disabled = 0b11 << 5,
    Breakbeam = 1 << 5,
    Immediate = 1 << 6,
};

/**
 * Convert a TrigerMode to a String
 * 
 * @param trigger TriggerMode to translate
 * 
 * @return Stromg format of trigger
 */
String triggermode_to_str(TriggerMode trigger);

/**
 * The Kicker Command is the command sent to the kicker
 *
 * The KickerCommand has the following format:
 * +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+
 * |    15   |    14   |    13   |    12   |    11   |    10   |    9    |    8    |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
 * +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+
 * | filler                                                                        | mode    | trigger           | charge? | strength                              |
 * +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+
 * 
 * Size = 16 bits
 */
struct KickerCommand {
    // Type of kick to be performed by kicker
    ShootMode shoot_mode = Kick;
    // Trigger for the kicker to kick/chip
    TriggerMode trigger_mode = Disabled;
    // Strength of kick linearly scaling 0-15
    uint8_t kick_strength = 0;
    // Whether or not kicker is allowed to charge
    bool charge_allowed = false;

    /**
     * Converts kicker command into SPI formated byte to send
     * 
     * @return Byte to be sent to kicker
     */
    uint16_t pack();

    /**
     * Convert kicker command into a readible String format
     * 
     * @return String representation of kicker command
     */
    const String to_string();
};

// Kicker error enum from kicker code
enum KickerError {
    None = 0b11111111, // Should never be used technically, only to send ok to teensy
    ChargeTimeout = 0b10011,
    OverVoltage = 0b01101,
    MajorOverVoltage = 0b11111,
    ChargeKickOverlap = 0b01010,
    BreakbeamBlockage = 0b00011,
    NoCharge = 0b11100,
    NoDischarge = 0b01011,
    ContinuousCharging = 0b10111,
    ContinuousDischarge = 0b00100,
    Unknown = 0b10101,
};

const char* kicker_error_to_str(KickerError e);


/**
 * The Kicker response is the returned from the kicker upon service
 *
 * The response has the following format:
 * +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+
 * |    15   |    14   |    13   |    12   |    11   |    10   |    9    |    8    |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
 * +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+
 * | error_code                                                                    | break?  | cap_voltage / 2                                                     |
 * +---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+
 * 
 * Size = 16 bits
 */

class Kicker {
public:
    // Voltage of the kicker capacitors
    uint8_t current_voltage = 0;
    // True if kicker breakbeam currently broken
    bool ball_sensed = false;
    // False if kicker has errored or stopped responding
    bool healthy = false;
    // Current kicker error
    KickerError error = None;

    /**
     * Kicker constructor
     * 
     * @param spi SPI bus the kicker is attached to
     * @param settings SPISettings to use for transactions
     * @param cs_pin Chip-select pin for kicker
     * @param reset_pin Reset pin for kicker
     * @param miso_pin Optional non-default MISO pin. Pass -1 to leave default
     * 
     * @note Currently we only overwrite miso pin, may need mosi pin as well in future
     */
    Kicker(SPIClass &spi, SPISettings settings, uint8_t cs_pin, uint8_t reset_pin, int8_t miso_pin = -1);

    /**
     * Initializes SPI and cs/reset pins
     */
    void begin();

    /**
     * Packs and sends command to Kicker and then updates state from response
     * 
     * @param command KickerCommand to send
     */
    void service(KickerCommand command);

    /**
     * Drives reset pin low for 10ms to reset kicker board
     */
    void reset();

    bool reset_error();

    /**
     * Convert kicker state information into a readible String format
     * 
     * @return String representation of kicker state
     */
    const String state_string();

private:
    SPIClass &_spi;
    SPISettings _settings;
    uint8_t _cs_pin;
    uint8_t _reset_pin;
    int8_t _miso_pin;

    /**
     * Updates kicker state information from raw SPI input
     * 
     * @param raw SPI reponse from kicker servicing
     */
    void update_state(uint16_t raw);
};