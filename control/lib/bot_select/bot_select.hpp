// Robot ID and color selector
// Current implementation is 4 toggle switches 1-4 where 1 is color and 2-4 are ID with 4 as LSB

#pragma once

#include <Arduino.h>
#include "types.hpp"
#include "pins.hpp"

/**
 * Initialize pins/internal state for robot id/team selector
 */
void init_botsel();

/**
 * Read the current team selected
 * 
 * @return Robot team 
 */
Team read_team();

/**
 * Read the current id selected
 * 
 * @return Robot id
 */
uint8_t read_id();