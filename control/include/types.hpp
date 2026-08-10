#pragma once

enum Team {
    Blue = 0,
    Yellow = 1,
};

enum class RobotError {
    NoError,
    RadioError,
    RecoverableKicker,
    UnrecoverableKicker,
    BatteryUndervolt,
};