#pragma once

#include <Arduino.h>

enum ShootMode {
    Kick = 0,
    Chip = 1 << 7,
};

String shootmode_to_str(ShootMode type);

enum TriggerMode {
    Disabled = 0b11 << 5,
    Breakbeam = 1 << 5,
    Immediate = 1 << 6,
};

String triggermode_to_str(TriggerMode trigger);

struct KickerCommand {
    ShootMode kick_type = Kick;
    TriggerMode kick_trigger = Disabled;
    uint8_t kick_strength = 0;
    bool charge_allowed = false;

    uint8_t pack();
    String to_string();
};

struct KickerState {
    uint8_t current_voltage;
    bool ball_sensed;
    bool healthy;

    KickerState(uint8_t);
    String to_string();
};