#pragma once

#include <Arduino.h>

/// @brief Type of kick to be performed by the kicker
enum ShootMode {
    Kick = 0,
    Chip = 1 << 7,
};

/// @brief Convert a ShootMode to a String
/// @param type ShootMode to translate
/// @return String format of mode
String shootmode_to_str(ShootMode mode);

/// @brief What trigger will activate the kicker: None, Breakbeam, or Immediate
enum TriggerMode {
    Disabled = 0b11 << 5,
    Breakbeam = 1 << 5,
    Immediate = 1 << 6,
};

/// @brief Convert a TrigerMode to a String
/// @param trigger TriggerMode to translate
/// @return Stromg format of trigger
String triggermode_to_str(TriggerMode trigger);

// TODO: Copy and reformat table from rust docs
struct KickerCommand {
    // Type of kick to be performed by kicker
    ShootMode shoot_mode = Kick;
    // Trigger for the kicker to kick/chip
    TriggerMode trigger_mode = Disabled;
    // Strength of kick linearly scaling 0-15
    uint8_t kick_strength = 0;
    // Whether or not kicker is allowed to charge
    bool charge_allowed = false;

    /// @brief Converts kicker command into SPI formated byte to send
    /// @return Byte to be sent to kicker
    uint8_t pack();
    /// @brief Convert kicker command into a readible String format
    /// @return String representation of kicker command
    String to_string();
};

// TODO: Copy and reformat table from rust docs
struct KickerState {
    // Voltage of the kicker capacitors
    uint8_t current_voltage;
    // True if kicker breakbeam currently broken
    bool ball_sensed;
    // False if kicker has errored or stopped responding
    bool healthy;

    /// @brief Creates a KickerState from the byte returned by kicker SPI
    /// @param state Byte response from the kicker
    KickerState(uint8_t state);
    /// @brief Converts kicker state into a readible String format
    /// @return String representation of kicker state
    String to_string();
};