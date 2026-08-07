/*
TODO: Add state machine for different operating modes
TODO: More tests, continue porting old ones and add new ones
TODO: Clean up debug printing so it isn't slowing ISR functions
*/

#include "main.hpp"

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
Kicker kicker(SPI1, settings, KICKER_CSN_PIN, KICKER_RESETN_PIN, KICKER_MISO_PIN);
// I2C Display
Display display = Display();
// Radio
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN, 5000000);
// Bot select
BotSelect bot_select = BotSelect();

/// Managers
// Motion
MotionControl motion_controller = MotionControl();
// Interrupt Timers
IntervalTimer motion_timer;
IntervalTimer kicker_timer;
IntervalTimer low_priority_timer;

/// Local Variables
// Current robot status
RobotStatusMessage status;
// Current command to be executed
ControlMessage control_message;
// # of times battery undervoltage detected
uint8_t batt_uvlo_counter = 0;
// Loop count just for screen swapping rn
uint32_t iteration = 0;
// New command from radio interrupt
volatile bool new_command = false;
// Timestamp (ms) of last command to check radio timeout
uint32_t last_command = 0;
// Kicker voltage
uint8_t kicker_voltage = 0;
// For timing profiling
elapsedMicros us;
// Radio timeout
bool radio_timeout = true;
// History of good/bad sends
bool radio_acks[100];
// Index of radio history array
uint8_t radio_acks_idx = 0;
// Temp toggle to stop just motion and kicker isrs
bool idle = false;
// Current error
RobotError current_error = NoError;

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
  // End Initialize Motor Board //

  // Initialize Kicker //
  kicker.begin();
  // End Initialize Kicker //

  // Initialize Display //
  display.begin(1000000);
  // End Initialize Display //

  // Initialize Serial //
  Serial.begin(115200);
  // Wait 3 seconds or until serial connected
  for (int i = 0; i < 3; i++) {
    if (Serial.available()) break;
    display.clear_buffer();
    display.draw_startup(i + 1);
    display.send_buffer();
    delay(1000);
  }
  // End Initialize Serial //
  
  // Initialize Bot Select //
  bot_select.begin();
  status.team = bot_select.team;
  status.robot_id = bot_select.id;
  // End Initialize Bot Select //

  // Initialize radio //
  if(!radio.begin()) {
    Serial.println("Radio Init Failure!");
    current_error = RadioError;
  }

  // Tie interrupt to radio receive
  pinMode(RADIO_IRQ_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RADIO_IRQ_PIN), receive_command, FALLING);
  radio.setStatusFlags(RF24_RX_DR); // Only care about data ready

  // Ready radio to receive commands
  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(CHANNEL);
  radio.openWritingPipe(BASE_STATION_ADDRESSES[status.team]);
  radio.openReadingPipe(1, ROBOT_RADIO_ADDRESSES[status.team][status.robot_id]);
  radio.setPayloadSize(CONTROL_MESSAGE_SIZE);
  radio.startListening();

  if (DEBUG) radio.printDetails();
  // Initialize acks array to true
  memset(radio_acks, true, sizeof(radio_acks));
  // End Initialize Radio //

  // Initialize Interrupt Timers //
  motion_timer.begin(motion_isr, MOTION_FREQ_US);
  motion_timer.priority(64);
  kicker_timer.begin(kicker_isr, KICKER_FREQ_US);
  kicker_timer.priority(96);
  low_priority_timer.begin(low_priority_isr, LOW_PRIO_FREQ_US);
  low_priority_timer.priority(192);
  // End Initialize Interrupt Timers //
}

// Main control loop
void loop() {
  us = 0;

  // Check errors
  if (current_error != NoError) {
    // Stop motion and kicker
    idle = true;
    error_handler(current_error);
  } else {
    // Movement allowed in event of error correction
    idle = false;
  }

  // Check for radio timeout
  radio_timeout = millis() - last_command > DIE_TIME_MS;

  /// Service the radio
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

      ControlMessage temp_ctrl;
      temp_ctrl.unpack(data);
      // Safe write
      noInterrupts();
      // Overwrite current command with new command
      control_message = temp_ctrl;
      interrupts();
      
      if (DEBUG) Serial.println(control_message.to_string());
      // Send status response
      if (DEBUG) Serial.println("Sending response!");
      uint8_t response[ROBOT_STATUS_SIZE];
      // Immediate confirmation response
      status.kick_status = control_message.trigger_mode != Disabled;
      RobotStatusMessage temp_status;
      // Safe copy
      noInterrupts();
      temp_status = status;
      interrupts();
      temp_status.pack(response);
      radio.stopListening();
      radio.setPayloadSize(ROBOT_STATUS_SIZE);
      bool ack = radio.write(&response, ROBOT_STATUS_SIZE);
      // Update history
      radio_acks[radio_acks_idx] = ack;
      radio_acks_idx = (radio_acks_idx + 1) % 100;
      if (DEBUG) Serial.println((ack ? "Good send!" : "Send Fail!"));
      if (DEBUG) Serial.printf("Radio success rate: %d%%\n", acks_to_percent(radio_acks));
      radio.setPayloadSize(CONTROL_MESSAGE_SIZE);
      radio.startListening();
    }
    // Trash first radio movement command after timeout to prevent jolts
    // TODO: better test if needed, edge case possible
    if (radio_timeout) {
      if (DEBUG) Serial.printf("Trashed Packet: X: %d | Y: %d | W: %d\n", control_message.body_x, control_message.body_y, control_message.body_w);
      control_message.body_x = 0;
      control_message.body_y = 0;
      control_message.body_w = 0;
    }
    last_command = millis();
  }
  
  if (DEBUG) Serial.printf("Loop time: %lu us\n", (uint32_t) us);
}

// Radio interrupt
void receive_command() {
  new_command = true;
}

// Motion control interrupt
void motion_isr() {
  if (idle) return;
  /// Service the motors
  // Calculate wheel velocities, zero if past die time
  Vector3f body_velocities = (radio_timeout ? Vector3f::Zero() : control_message.get_velocity());
  Vector4i wheel_velocities = motion_controller.body_to_wheels(body_velocities);
  Vector4i read_velocities = Vector4i::Zero();
  // Send commands to motor controllers
  // TODO: Find real source of order reversal
  read_velocities(3) = motors[0].send_and_read(wheel_velocities(3));
  read_velocities(2) = motors[1].send_and_read(wheel_velocities(2));
  read_velocities(1) = motors[2].send_and_read(wheel_velocities(1));
  read_velocities(0) = motors[3].send_and_read(wheel_velocities(0));
  if (DEBUG) Serial.printf("Body Velocities: (%.3f, %.3f, %.3f)\n", body_velocities(0), body_velocities(1), body_velocities(2));
  if (DEBUG) Serial.printf("Wheel Velocities: (%d, %d, %d, %d)\n", wheel_velocities(0), wheel_velocities(1), wheel_velocities(2), wheel_velocities(3));
  if (DEBUG) Serial.printf("Read Velocities: (%d, %d, %d, %d)\n", read_velocities(0), read_velocities(1), read_velocities(2), read_velocities(3));
}

// Kicker control interrupt
void kicker_isr() {
  if (idle) return;
  /// Service the kicker
  // Construct command 
  KickerCommand kcommand;
  // Disallow charging on radio timeout
  kcommand.charge_allowed = !radio_timeout;
  kcommand.kick_strength = control_message.kick_strength;
  kcommand.trigger_mode = control_message.trigger_mode;
  kcommand.shoot_mode = control_message.shoot_mode;
  // Send command
  kicker.service(kcommand);
  // Update status
  status.kick_healthy = kicker.healthy;
  status.ball_sense_status = kicker.ball_sensed;
  kicker_voltage = kicker.current_voltage;
  if (DEBUG) Serial.print("Kicker Response: ");
  if (DEBUG) Serial.println(kicker.state_string());
  if (kicker.error == BreakbeamBlockage) {
    current_error = RecoverableKicker;
  } else if (kicker.error != KickerError::None && millis() >= 5000) {
    current_error = UnrecoverableKicker;
  }
}

// Random things interrupt
void low_priority_isr() {
  iteration++;
  /// Poll battery voltage
  uint16_t raw_batt = analogRead(BATTERY_SENSE_PIN);
  float battery_voltage = raw_batt * 3.3 / 1023.0;
  if (DEBUG) Serial.printf("Battery Voltage: %.2f\n", battery_voltage);
  // Maximum Voltage of batteries is roughly 2.69, so we're making a random linear interpolation between the max and min voltage
  status.battery_percent = int((battery_voltage - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE) * 100);
  if (DEBUG) Serial.printf("Battery Percent: %d\n", status.battery_percent);
  // Shut down if battery voltage too low
  if (battery_voltage < MIN_BATTERY_VOLTAGE) {
    batt_uvlo_counter++;
    if (batt_uvlo_counter > BATT_UVLO_THRESHOLD) {
      Serial.println("Undervoltage Detected!");
      current_error = BatteryUndervolt;
      // Force shutdown at too low a battery
      if (battery_voltage < UNSAFE_BATTERY_VOLTAGE) {
        kill_self();
      }
    }
  } else {
    batt_uvlo_counter = 0;
  }

  /// Update screen
  // TODO: Maybe better idea than cycling between the two screens
  // however it is currently built out to accept more
  if (current_error != NoError) return;
  if (iteration != 0 && iteration % 10 == 0) {
    display.next_window();
  }
  // Currently screen takes ~10ms to update so it gets to live in the main loop
  display.clear_buffer();
  display.update_info(status, !radio_timeout, kicker_voltage, acks_to_percent(radio_acks));
  display.draw_header();
  display.draw_window();
  display.send_buffer();
}

// Universal error handler
void error_handler(RobotError e) {
  // Alert to screen
  display.clear_buffer();
  display.draw_error(e);
  display.send_buffer();

  // Stop motors
  for (auto& motor : motors) {
    motor.send_command(0);
  }

  // Stop kicker
  kicker.service(KickerCommand());

  // Error specific handling
  switch (e) {
    case RadioError:
      Serial.println("RADIO ERROR");
    break;
    case RecoverableKicker:
      Serial.println("RECOVERALBE KICKER ERROR ATTEMPTING RESTART");
      if (kicker.reset_error()) {
        Serial.println("Recovered kicker!");
        current_error = NoError;
      } else {
        current_error = UnrecoverableKicker;
      }
    break;
    case UnrecoverableKicker:
      Serial.println("UNRECOVERABLE KICKER ERROR");
    break;
    case BatteryUndervolt:
      Serial.println("BATTERY UNDERVOLTAGE ERROR!");
    break;
    default:
      Serial.println("UNKNOWN ERROR!");
    break;
  }

  delay(500);
} 

// Safe robot shutdown ending with killing motor board
void kill_self() {
  // Stop interrupts
  noInterrupts();
  // Stop motors
  Serial.println("Stopping motors!");
  for (auto& motor : motors) {
    motor.send_command(0);
  }

  // Kick to discharge
  KickerCommand kcommand = {
    Kick,
    Immediate,
    8,
    false
  };
  for (size_t i = 0; i < 4; i++) {
    kicker.service(kcommand);
    delay(250);
  }

  // Kill Power
  Serial.println("Killing Motor Board!");
  digitalWrite(KILLN_PIN, LOW);

  // Prevent further actions
  while(1) {delay(1);}
}
