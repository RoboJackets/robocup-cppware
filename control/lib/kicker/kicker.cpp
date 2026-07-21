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
    command |= shoot_mode;
    command |= trigger_mode;
    command |= kick_strength & 0xF;
    if (charge_allowed) {
        command |= 1 << 4;
    }
    return command;
}

String KickerCommand::to_string() {
    return "Type: " + shootmode_to_str(shoot_mode) + " | Trigger: " + triggermode_to_str(trigger_mode) + " | Strength: " + String(kick_strength) + " | Charge Allowed: " + String(charge_allowed);
}


KickerState::KickerState(uint8_t raw) {
    current_voltage = (raw & 0x7F) << 1;
    ball_sensed = (raw & (1 << 7)) != 0;
    healthy = raw != 0;
}

String KickerState::to_string() {
    return String("Voltage: ") + current_voltage + " | Ball Sensed: " + ball_sensed + " | Healthy: " + healthy;
}


Kicker::Kicker(SPIClass &spi, SPISettings settings, uint8_t cs_pin, uint8_t reset_pin, int8_t miso_pin)
    : state(0), _spi(spi), _settings(settings), _cs_pin(cs_pin), _reset_pin(reset_pin), _miso_pin(miso_pin) {}

void Kicker::begin() {
    // Check for non-default spi pin
    if (_miso_pin >= 0) {
        _spi.setMISO(_miso_pin);
    }
    // Initialize SPI
    _spi.begin();

    pinMode(_cs_pin, OUTPUT);
    digitalWrite(_cs_pin, HIGH);

    pinMode(_reset_pin, OUTPUT);
    digitalWrite(_reset_pin, HIGH);
}

void Kicker::service(KickerCommand command) {
    _spi.beginTransaction(_settings);
    digitalWrite(_cs_pin, LOW);
    uint8_t response = _spi.transfer(command.pack());
    digitalWrite(_cs_pin, HIGH);
    _spi.endTransaction();
    state = KickerState(response);
}

void Kicker::reset() {
    digitalWrite(_reset_pin, LOW);
    delay(10);
    digitalWrite(_reset_pin, HIGH);
}