#include <Arduino.h>
#include <SPI.h>

#include "pins.hpp"
#include "types.hpp"
#include "config.hpp"
#include "bot_select.hpp"
#include "motors.hpp"
#include "kicker.hpp"

// Temp (probably) vars
Team team;
uint8_t id;

MotorController motors[MOTOR_COUNT] = {
    MotorController(Serial1),
    MotorController(Serial2),
    MotorController(Serial6),
    MotorController(Serial7),
    MotorController(Serial5),
};

int32_t motor_velocities[MOTOR_COUNT] = {};

SPISettings settings(2000000, MSBFIRST, SPI_MODE3);

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

  // Start uart for all motors
  for (auto& motor : motors) {
    motor.begin();
  }
  // End Initialize Motor Board
  
  // Initialize Bot Select //
  init_botsel();
  team = read_team();
  id = read_id();
  // End Initialize Bot Select //

  // Initialize Kicker //
  SPI1.begin();
  pinMode(KICKER_CSN_PIN, OUTPUT);
  digitalWrite(KICKER_CSN_PIN, HIGH);
  // End Initialize Kicker //
}

// Main control loop
void loop() {
  Serial.printf("Team: %d | ID: %d\n", team, id);

  KickerCommand new_command;
  new_command.charge_allowed = true;
  new_command.kick_strength = 5;
  new_command.kick_trigger = Breakbeam;

  SPI1.beginTransaction(settings);
  digitalWrite(KICKER_CSN_PIN, LOW);

  uint8_t response = SPI1.transfer(new_command.pack());

  digitalWrite(KICKER_CSN_PIN, HIGH);
  SPI1.endTransaction();

  KickerState ks = KickerState(response);
  Serial.println("KS: " + ks.to_string());

  delay(100);
}

// Safe robot shutdown ending with killing motor board
// TODO: exchange for a flag and trigger in main loop
void kill_self() {
  Serial.println("Killing Motor Board!");
  digitalWrite(KILL_N_PIN, LOW);
}
