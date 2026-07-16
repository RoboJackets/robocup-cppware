#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <RF24.h>
#undef printf
#include <ArduinoEigen.h>

#include "pins.hpp"
#include "types.hpp"
#include "config.hpp"
#include "bot_select.hpp"
#include "motors.hpp"
#include "kicker.hpp"
#include "radio.hpp"
#include "motion.hpp"


void kill_self();
void receive_command();
void error_handler(RobotError);