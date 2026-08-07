#include "bot_select.hpp"


void BotSelect::begin() {
    pinMode(BOTSEL_PIN0, INPUT_PULLUP);
    pinMode(BOTSEL_PIN1, INPUT_PULLUP);
    pinMode(BOTSEL_PIN2, INPUT_PULLUP);
    pinMode(BOTSEL_PIN3, INPUT_PULLUP);
    delay(1);
    BotSelect::update();
}

void BotSelect::update() {
    team = BotSelect::read_team();
    id = BotSelect::read_id();
}

Team BotSelect::read_team() {
    // Active low
    if (!digitalRead(BOTSEL_PIN0)) {
        return Yellow;
    }
    return Blue;
}

uint8_t BotSelect::read_id() {
    uint8_t id = 0;
    // Active low
    if (!digitalRead(BOTSEL_PIN3)) id += 1;
    if (!digitalRead(BOTSEL_PIN2)) id += 2;
    if (!digitalRead(BOTSEL_PIN1)) id += 4;
    return id;
}