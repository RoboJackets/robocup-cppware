#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <RF24.h>
#undef printf
#include <ArduinoEigen.h>
using namespace Eigen;

#include "pins.hpp"
#include "types.hpp"
#include "config.hpp"
#include "bot_select.hpp"
#include "motors.hpp"
#include "kicker.hpp"
#include "radio.hpp"
#include "motion.hpp"
#include "display.hpp"

void power_switch_interrupt();
void kill_self();
void receive_command();
void error_handler(RobotError);