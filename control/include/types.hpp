#pragma once

enum Team {
    Blue = 0,
    Yellow = 1,
};

enum RobotError {
    NoError,
    RadioError,
    RecoverableKicker,
    UnrecoverableKicker,
    BatteryUndervolt,
};