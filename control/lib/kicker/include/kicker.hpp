#pragma once

#include <Arduino.h>

enum KickType {
    Kick = 0,
    Chip = 1 << 7,
};

String kicktype_to_str(KickType type);

enum KickTrigger {
    Disabled = 0b11 << 5,
    Breakbeam = 1 << 5,
    Immediate = 1 << 6,
};

String kicktrigger_to_str(KickTrigger trigger);

struct KickerCommand {
    KickType kick_type = Kick;
    KickTrigger kick_trigger = Disabled;
    uint8_t kick_strength = 0;
    bool charge_allowed = false;

    uint8_t pack() {
        uint8_t command = 0;
        command |= kick_type;
        command |= kick_trigger;
        command |= kick_strength & 0xF;
        if (charge_allowed) {
            command |= 1 << 4;
        }
        return command;
    }

    String to_string() {
        return "Type: " + kicktype_to_str(kick_type) + " | Trigger: " + kicktrigger_to_str(kick_trigger) + " | Strength: " + String(kick_strength) + " | Charge Allowed: " + String(charge_allowed);
    }
};

struct KickerState {
    uint8_t current_voltage;
    bool ball_sensed;
    bool healthy;

    KickerState(uint8_t raw) {
        current_voltage = (raw & 0x7F) << 1;
        ball_sensed = (raw & (1 << 7)) != 0;
        healthy = raw != 0;
    }

    String to_string() {
        return String("Voltage: ") + current_voltage + " | Ball Sensed: " + ball_sensed + " | Healthy: " + healthy;
    }
};