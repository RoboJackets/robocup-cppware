#include <Arduino.h>

#include "pins.hpp"
#include "types.hpp"
#include "config.hpp"
#include "bot_select.hpp"

// Temp (probably) vars
Team team;
uint8_t id;

void kill_self();

void setup() {
  Serial.begin(115200);

  // Initialize Motor Board //
  pinMode(MOTOR_EN_PIN, OUTPUT);
  digitalWrite(MOTOR_EN_PIN, HIGH);

  pinMode(KILL_N_PIN, OUTPUT);
  digitalWrite(KILL_N_PIN, HIGH);

  pinMode(POWER_SWITCH_PIN, INPUT);
  delay(15);
  attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), kill_self, FALLING);

  // TODO UART connections
  // End Initialize Motor Board
  
  // Initialize Bot Select //
  init_botsel();
  team = read_team();
  id = read_id();
  // End Initialize Bot Select //
}

// Main control loop
void loop() {
  Serial.printf("Team: %d | ID: %d\n", team, id);
  if (!digitalRead(POWER_SWITCH_PIN)) {
    Serial.println("POWAAAA");
  }
  delay(100);
}

// Safe robot shutdown ending with killing motor board
void kill_self() {
  Serial.println("Killing Motor Board!");
  digitalWrite(KILL_N_PIN, LOW);
}
