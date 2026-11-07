
#include <Servo.h>

// ================= FLAME SENSORS =================
// Sensor output LOW = Fire detected
#define FLAME_CENTER 2
#define FLAME_LEFT   3
#define FLAME_RIGHT  4

// ================= MQ-2 GAS SENSOR =================
#define GAS_SENSOR A1

// ================= L293D MOTOR DRIVER =================
#define ENA 6
#define IN1 7
#define IN2 9

#define ENB 5
#define IN3 11
#define IN4 12

// ================= OTHER COMPONENTS =================
#define BUZZER 8
#define RELAY_PIN A0
#define SERVO_PIN 13

// Active LOW relay
#define PUMP_ON  LOW
#define PUMP_OFF HIGH

// ================= SETTINGS =================
#define MOTOR_SPEED 120
#define SLOW_SPEED  90
#define APPROACH_TIME 250
#define SPRAY_TIME   5000

Servo waterServo;

// ================= SENSOR FUNCTIONS =================
bool centerFire() {
  return digitalRead(FLAME_CENTER) == LOW;
}

bool leftFire() {
  return digitalRead(FLAME_LEFT) == LOW;
}

bool rightFire() {
  return digitalRead(FLAME_RIGHT) == LOW;
}

bool anyFire() {
  return centerFire() || leftFire() || rightFire();
}

bool gasDetected() {
  return digitalRead(GAS_SENSOR) == LOW;
}

// ================= MOTOR CONTROL =================
// Assumed forward directions:
// Left motor: IN1 HIGH, IN2 LOW
// Right motor: IN3 LOW, IN4 HIGH





void moveForward() {
  // Right side forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Left side forward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, 180);
  analogWrite(ENB, 180);
}

void forwardLeft() {
  // Right side forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Left side backward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, 180);
  analogWrite(ENB, 180);
}

void forwardRight() {
  // Right side backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Left side forward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, 180);
  analogWrite(ENB, 180);
}


void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}


// ================= PUMP CONTROL =================
void pumpOn() {
  digitalWrite(RELAY_PIN, PUMP_ON);
}

void pumpOff() {
  digitalWrite(RELAY_PIN, PUMP_OFF);
}

// ================= SAFE STOP =================
void emergencyStop() {
  stopMotors();
  pumpOff();
  digitalWrite(BUZZER, LOW);
}

// ================= APPROACH FIRE =================

void approachFire() {
  unsigned long startTime = millis();

  while (millis() - startTime < APPROACH_TIME) {

    if (gasDetected() || !anyFire()) {
      stopMotors();
      return;
    }

    bool C = centerFire();
    bool L = leftFire();
    bool R = rightFire();

    if (C) {
      moveForward();
    }
    else if (L && !R) {
      forwardLeft();
    }
    else if (R && !L) {
      forwardRight();
    }
    else if (L && R) {
      moveForward();
    }

    delay(20);
  }

  stopMotors();
}


// ================= EXTINGUISH FIRE =================
void extinguishFire() {
  stopMotors();

  unsigned long startTime = millis();

  while (millis() - startTime < SPRAY_TIME) {

    // Stop spraying if fire disappears or gas is detected
    if (gasDetected() || !anyFire()) {
      break;
    }

    pumpOn();

    // Sweep water nozzle
    waterServo.write(50);
    delay(250);

    if (gasDetected() || !anyFire()) {
      break;
    }

    waterServo.write(90);
    delay(250);

    if (gasDetected() || !anyFire()) {
      break;
    }

    waterServo.write(130);
    delay(250);

    if (gasDetected() || !anyFire()) {
      break;
    }

    waterServo.write(90);
    delay(250);
  }

  pumpOff();
  waterServo.write(90);
  stopMotors();
}

// ================= SETUP =================
void setup() {
  pinMode(FLAME_CENTER, INPUT_PULLUP);
  pinMode(FLAME_LEFT, INPUT_PULLUP);
  pinMode(FLAME_RIGHT, INPUT_PULLUP);

  pinMode(GAS_SENSOR, INPUT_PULLUP);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(BUZZER, LOW);
  pumpOff();
  stopMotors();

  waterServo.attach(SERVO_PIN);
  waterServo.write(90);

  delay(1000);
}

// ================= MAIN LOOP =================
void loop() {

  // 1. Gas detected: Stop robot, stop pump and activate alarm
  if (gasDetected()) {
    stopMotors();
    pumpOff();
    gasAlarm();
    delay(100);
    return;
  }

  // 2. No gas detected: Turn off gas alarm
  stopAlarm();

  // 3. Read flame sensors
  bool C = centerFire();
  bool L = leftFire();
  bool R = rightFire();

  // 4. No fire detected: Stop everything
  if (!C && !L && !R) {
    emergencyStop();
    stopAlarm();
    delay(100);
    return;
  }

  // 5. Fire detected: Activate buzzer
  digitalWrite(BUZZER, HIGH);

  // 6. Move toward the detected fire direction
  approachFire();

  // 7. Stop motors after approaching
  stopMotors();

  // 8. Check gas again before operating pump
  if (gasDetected()) {
    stopMotors();
    pumpOff();
    gasAlarm();
    return;
  }

  // 9. If fire is still detected, activate water pump
  if (anyFire()) {
    extinguishFire();
  }

  // 10. Stop motors and pump
  stopMotors();
  pumpOff();

  delay(100);
}


// ================= GAS ALARM =================
void gasAlarm() {
  digitalWrite(BUZZER, HIGH);
}

void stopAlarm() {
  digitalWrite(BUZZER, LOW);
}