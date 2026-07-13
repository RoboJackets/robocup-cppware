#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <RF24.h>
#undef printf

#include "pins.hpp"
#include "types.hpp"
#include "config.hpp"
#include "bot_select.hpp"
#include "motors.hpp"
#include "kicker.hpp"
#include "radio.hpp"


struct RxPacket {
  uint8_t data[10];
};

struct TxPacket {
  uint8_t data[3];
};

// Temp (probably) vars
RobotStatusMessage status;

MotorController motors[MOTOR_COUNT] = {
    MotorController(Serial1),
    MotorController(Serial2),
    MotorController(Serial6),
    MotorController(Serial7),
    MotorController(Serial5),
};

int32_t motor_velocities[MOTOR_COUNT] = {};

SPISettings settings(2000000, MSBFIRST, SPI_MODE3);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN, 5000000);
volatile bool new_command = false;

void kill_self();
void receive_command();

void setup() {
  Serial.begin(115200);
  while(!Serial.available() && millis() < 3000){delay(1);}

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
  status.team = read_team();
  status.robot_id = read_id();
  // End Initialize Bot Select //

  // Initialize Kicker //
  SPI1.begin();
  pinMode(KICKER_CSN_PIN, OUTPUT);
  digitalWrite(KICKER_CSN_PIN, HIGH);
  // End Initialize Kicker //

  // Initialize Screen //
  u8g2.begin();
  // End Initialize Screen //

  // Initialize radio //
  if(!radio.begin()) {
    Serial.println("Radio Init Failure!");
    while (1);
  }

  // Tie interrupt to radio receive
  pinMode(RADIO_IRQ_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RADIO_IRQ_PIN), receive_command, FALLING);
  radio.setStatusFlags(RF24_RX_DR);

  // Ready radio to receive commands
  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(CHANNEL);
  radio.openWritingPipe(BASE_STATION_ADDRESSES[status.team]);
  radio.openReadingPipe(1, ROBOT_RADIO_ADDRESSES[status.team][status.robot_id]);
  radio.setPayloadSize(CONTROL_MESSAGE_SIZE);
  radio.startListening();

  if (DEBUG) radio.printDetails();
  // End Initialize Radio
}

uint32_t iter = 0;
// Main control loop
void loop() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  char buf[32];
  snprintf(buf, sizeof(buf), "Team: %d | ID: %d | Itr: %ld", status.team, status.robot_id, iter);
  u8g2.drawStr(0,10, buf);
  iter++;

  // Read new command if available
  if (new_command) {
    Serial.println("New Command!");
    new_command = false;
    radio.clearStatusFlags();

    if (radio.available()) {
      uint8_t data[CONTROL_MESSAGE_SIZE];
      radio.read(&data, CONTROL_MESSAGE_SIZE);

      ControlMessage msg;
      msg.unpack(data);
      Serial.println(msg.to_string());

      Serial.println("Sending response!");
      uint8_t response[ROBOT_STATUS_SIZE];
      status.pack(response);
      radio.stopListening();
      radio.setPayloadSize(ROBOT_STATUS_SIZE);
      bool ack = radio.write(&response, ROBOT_STATUS_SIZE);
      if (ack) Serial.println("Good send!");
      radio.setPayloadSize(CONTROL_MESSAGE_SIZE);
      radio.startListening();
    }
  }
  
  u8g2.sendBuffer();	
  delay(10);
}

// Safe robot shutdown ending with killing motor board
// TODO: exchange for a flag and trigger in main loop
void kill_self() {
  Serial.println("Killing Motor Board!");
  digitalWrite(KILL_N_PIN, LOW);
}

void receive_command() {
  new_command = true;
}