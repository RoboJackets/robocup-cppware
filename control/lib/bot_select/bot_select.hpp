/*
Robot ID and color selector
Current implementation is 4 toggle switches 1-4 where 1 is color and 2-4 are ID number with 4 as LSB
*/

#pragma once

#include <Arduino.h>
#include "types.hpp"
#include "pins.hpp"

class BotSelect {
public:
    /**
     * Initialize pins/internal state for robot id/team selector
     * Performs one update to start states
     */
    void begin();

    /**
     * Updates the team and id from the peripheral
     */
    void update();

    // Last read team
    Team team = Blue;
    // Last read id
    uint8_t id = 0;
private:
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
};