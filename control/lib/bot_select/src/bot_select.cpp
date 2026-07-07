#include "bot_select.hpp"


void init_botsel() {
    pinMode(BOTSEL_PIN0, INPUT_PULLUP);
    pinMode(BOTSEL_PIN1, INPUT_PULLUP);
    pinMode(BOTSEL_PIN2, INPUT_PULLUP);
    pinMode(BOTSEL_PIN3, INPUT_PULLUP);
    delay(15);
}

Team read_team() {
    if (!digitalRead(BOTSEL_PIN0)) {
        return Yellow;
    }
    return Blue;
}

uint8_t read_id() {
    uint8_t id = 0;
    if (!digitalRead(BOTSEL_PIN3)) id += 1;
    if (!digitalRead(BOTSEL_PIN2)) id += 2;
    if (!digitalRead(BOTSEL_PIN1)) id += 4;
    return id;
}