#include "kicker.hpp"

String triggermode_to_str(TriggerMode trigger) {
    switch (trigger)
    {
    case Immediate:
        return "Immediate";
    case Breakbeam:
        return "Breakbeam";
    default:
        return "Disabled";
    }
}

String shootmode_to_str(ShootMode type) {
    switch (type)
    {
    case Chip:
        return "Chip";
    default:
        return "Kick";
    }
}

uint8_t KickerCommand::pack() {
    uint8_t command = 0;
    command |= kick_type;
    command |= kick_trigger;
    command |= kick_strength & 0xF;
    if (charge_allowed) {
        command |= 1 << 4;
    }
    return command;
}

String KickerCommand::to_string() {
    return "Type: " + shootmode_to_str(kick_type) + " | Trigger: " + triggermode_to_str(kick_trigger) + " | Strength: " + String(kick_strength) + " | Charge Allowed: " + String(charge_allowed);
}


KickerState::KickerState(uint8_t raw) {
    current_voltage = (raw & 0x7F) << 1;
    ball_sensed = (raw & (1 << 7)) != 0;
    healthy = raw != 0;
}

String KickerState::to_string() {
    return String("Voltage: ") + current_voltage + " | Ball Sensed: " + ball_sensed + " | Healthy: " + healthy;
}