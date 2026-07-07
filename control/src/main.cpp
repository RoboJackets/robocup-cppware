#include <Arduino.h>

#include <pins.hpp>

void setup() {
  Serial.begin(115200);

  // Initialize Motor Board //

  pinMode(MOTOR_EN_PIN, OUTPUT);
  digitalWrite(MOTOR_EN_PIN, HIGH);

  pinMode(KILL_N_PIN, OUTPUT);
  digitalWrite(KILL_N_PIN, HIGH);

  pinMode(POWER_SWITCH_PIN, INPUT);
  delay(15);

  // TODO UART connections

  // End Initialize Motor Board
  
}

void loop() {
  Serial.println("Loop :3");
  if (digitalRead(POWER_SWITCH_PIN)) {
    Serial.println("POWAAAA");
  }
  delay(100);
}

