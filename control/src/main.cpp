#include "main.hpp"
using namespace Eigen;

/// Peripherals
// Motors
MotorController motors[MOTOR_COUNT] = {
    MotorController(Serial1),
    MotorController(Serial2),
    MotorController(Serial6),
    MotorController(Serial7),
    MotorController(Serial5),
};
// Kicker SPI
SPISettings settings(2000000, MSBFIRST, SPI_MODE3);
// I2C Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// Radio
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN, 5000000);

/// Managers
// Motion
MotionControl motion_controller = MotionControl();

/// Vars
// Current robot status
RobotStatusMessage status;
// Current command to be executed
ControlMessage control_message;
// # of times battery undervoltage detected
uint8_t batt_uvlo_counter = 0;
// Loop count
uint32_t iteration = 0;
// New command from radio interrupt
volatile bool new_command = false;
// Timestamp of last command
uint32_t last_command = 0;


void setup() {
  // Initialize Motor Board //
  pinMode(MOTOR_EN_PIN, OUTPUT);
  digitalWrite(MOTOR_EN_PIN, HIGH);

  pinMode(KILLN_PIN, OUTPUT);
  digitalWrite(KILLN_PIN, HIGH);

  pinMode(POWER_SWITCH_PIN, INPUT);
  delay(15);
  attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), kill_self, FALLING);

  // Start uart for all motors
  for (auto& motor : motors) {
    motor.begin();
  }
  // End Initialize Motor Board

  // Initialize Kicker //
  // Non default MISO assignment
  SPI1.setMISO(KICKER_MISO_PIN);
  SPI1.begin();
  pinMode(KICKER_CSN_PIN, OUTPUT);
  digitalWrite(KICKER_CSN_PIN, HIGH);
  pinMode(KICKER_RESETN_PIN, OUTPUT);
  digitalWrite(KICKER_RESETN_PIN, HIGH);
  // End Initialize Kicker //

  // Initialize Serial //
  Serial.begin(115200);
  // Wait 3 seconds or until serial connected
  while(!Serial.available() && millis() < 3000){delay(1);}
  // End Initialize Serial //
  
  // Initialize Bot Select //
  init_botsel();
  status.team = read_team();
  status.robot_id = read_id();
  // End Initialize Bot Select //

  // Initialize Screen //
  u8g2.begin();
  // End Initialize Screen //

  // Initialize radio //
  if(!radio.begin()) {
    Serial.println("Radio Init Failure!");
    error_handler(RadioError);
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

// Main control loop
void loop() {
  // Check for radio timeout
  bool radio_timeout = millis() - last_command > DIE_TIME_MS;

  // Poll battery voltage
  uint16_t raw_batt = analogRead(BATTERY_SENSE_PIN);
  float battery_voltage = raw_batt * 3.3 / 1023.0;
  if (DEBUG) Serial.printf("Battery Voltage: %.2f\n", battery_voltage);
  // Maximum Voltage of batteries is roughly 2.69, so we're making a random linear interpolation between the max and min voltage
  status.battery_voltage = int((battery_voltage - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE) * 100);
  if (DEBUG) Serial.printf("Battery Percent: %d\n", status.battery_voltage);
  // Shut down if battery voltage too low
  if (battery_voltage < MIN_BATTERY_VOLTAGE) {
    batt_uvlo_counter++;
    if (batt_uvlo_counter > BATT_UVLO_THRESHOLD) {
      Serial.println("Undervoltage Detected!");
      kill_self();
    }
  }

  // Calculate wheel velocities, zero if past die time
  Vector3f body_velocities = (radio_timeout ? Vector3f::Zero() : control_message.get_velocity());
  Vector4i wheel_velocities = motion_controller.body_to_wheels(body_velocities);
  // Send commands to motor controllers
  for (size_t i = 0; i < 4; i++) {
    motors[i].send_command(wheel_velocities(i));
  }
  if (DEBUG) Serial.printf("Moving at (%d, %d, %d, %d)\n", wheel_velocities(0), wheel_velocities(1), wheel_velocities(2), wheel_velocities(3));

  
  /// Service the kicker
  // Construct command 
  KickerCommand kcommand;
  // Disallow charging on radio timeout
  kcommand.charge_allowed = !radio_timeout;
  kcommand.kick_strength = control_message.kick_strength;
  kcommand.trigger_mode = control_message.trigger_mode;
  kcommand.shoot_mode = control_message.shoot_mode;
  // Send command
  SPI1.beginTransaction(settings);
  digitalWrite(KICKER_CSN_PIN, LOW);
  uint8_t response = SPI1.transfer(kcommand.pack());
  digitalWrite(KICKER_CSN_PIN, HIGH);
  SPI1.endTransaction();
  // Process response
  KickerState kstate = KickerState(response);
  status.kick_healthy = kstate.healthy;
  status.ball_sense_status = kstate.ball_sensed;
  if (!status.kick_healthy) error_handler(KickerError);
  if (DEBUG) Serial.print("Kicker Response: ");
  if (DEBUG) Serial.println(kstate.to_string());


  // Read new command if available
  if (new_command) {
    if (DEBUG) Serial.println("New Command!");
    // Clear flags for new interrupts
    new_command = false;
    radio.clearStatusFlags();
    // Read command
    if (radio.available()) {
      uint8_t data[CONTROL_MESSAGE_SIZE];
      radio.read(&data, CONTROL_MESSAGE_SIZE);
      // Overwrite current command with new command
      control_message.unpack(data);
      if (DEBUG) Serial.println(control_message.to_string());
      // Send status response
      if (DEBUG) Serial.println("Sending response!");
      uint8_t response[ROBOT_STATUS_SIZE];
      status.pack(response);
      radio.stopListening();
      radio.setPayloadSize(ROBOT_STATUS_SIZE);
      bool ack = radio.write(&response, ROBOT_STATUS_SIZE);
      if (DEBUG && ack) Serial.println("Good send!");
      radio.setPayloadSize(CONTROL_MESSAGE_SIZE);
      radio.startListening();
    }
    last_command = millis();
  }
  
  // Update screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  char buf[32];
  snprintf(buf, sizeof(buf), "Team: %d | ID: %d", status.team, status.robot_id);
  u8g2.drawStr(0,10, buf);
  u8g2.sendBuffer();
}

// Safe robot shutdown ending with killing motor board
void kill_self() {
  Serial.println("Killing Motor Board!");
  digitalWrite(KILLN_PIN, LOW);
}

// Radio interrupt
void receive_command() {
  new_command = true;
}

// Universal error handler
void error_handler(RobotError e) {
  // Stop motors
  for (auto& motor : motors) {
    motor.send_command(0);
  }
  // Stop kicker
  SPI1.beginTransaction(settings);
  digitalWrite(KICKER_CSN_PIN, LOW);
  uint8_t _ = SPI1.transfer(KickerCommand().pack());
  digitalWrite(KICKER_CSN_PIN, HIGH);
  SPI1.endTransaction();

  // Error specific handling
  switch (e) {
    case RadioError:

    break;
    case KickerError:

    break;
    default:

    break;
  }
  // Unrecoverable by default
  while(1) {delay(1);}
} 