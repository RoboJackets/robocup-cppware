#pragma once

enum Team {
    Blue = 0,
    Yellow = 1,
};

// From front perspective
enum Motors {
    FR_MOTOR,
    BR_MOTOR,
    BL_MOTOR,
    L_MOTOR,
    DRIBBLER,
    MOTOR_COUNT,
};

enum RobotError {
    NoError,
    RadioError,
    KickerError,
    
};