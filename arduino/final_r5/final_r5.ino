#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
//final r5 code
// =========================
// BNO055
// =========================
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

// =========================
// Motor pins
// =========================
const int pwmLB = 16;
const int pwmRB = 4;
const int pwmLF = 2;
const int pwmRF = 15;

const int aInOne = 5;
const int aInTwo = 17;
const int aInOneRB = 19;
const int aInTwoRB = 18;
const int ainLF = 14;
const int binLF = 27;
const int ainRF = 12;
const int binRF = 13;

// =========================
// Ultrasonic pins
// =========================
const int TRIG_PIN_S = 23;
const int ECHO_PIN_S = 26;
const int TRIG_PIN_F = 32;
const int ECHO_PIN_F = 36;

// =========================
// Phototransistor
// =========================
const int sensorPin = 34;
const int ledPin = 25;

float thresholdVoltage = 0.2;
float referenceVoltage = 3.3;
int adcResolution = 4095;
int thresholdADC;

bool robotStarted = false;

// =========================
// Commands from Pi
// =========================
float linCmd = 0.0;
float angCmd = 0.0;

// =========================
// Tunables
// =========================
const int BASE_PWM = 170;
const int TURN_PWM = 180;

const float LIN_DEADBAND = 0.08;
const float ANG_DEADBAND = 0.10;

// Ultrasonic thresholds
const float FRONT_STOP_CM = 25.0;     // stop forward below this
const float FRONT_SLOW_CM = 45.0;     // optional slow zone
const float SIDE_WARN_CM  = 20.0;     // just debug for now

// =========================
// Helpers
// =========================
int clampPWM(int value) {
  if (value < 0) return 0;
  if (value > 255) return 255;
  return value;
}

// =========================
// Ultrasonic
// =========================
float ultrasonic_sensor(int TRIG_PIN, int ECHO_PIN) {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration_us = pulseIn(ECHO_PIN, HIGH, 30000);  // 30 ms timeout

  if (duration_us == 0) {
    return -1.0;  // invalid reading
  }

  float distance_cm = 0.017 * duration_us;
  return distance_cm;
}

// =========================
// Motor functions / finalmovemnt 
// =========================
void moveForward(int pwm) {
  digitalWrite(aInOne, HIGH);
  digitalWrite(aInTwo, LOW);
  analogWrite(pwmLB, pwm);

  digitalWrite(aInOneRB, HIGH);
  digitalWrite(aInTwoRB, LOW);
  analogWrite(pwmRB, pwm);

  digitalWrite(ainLF, HIGH);
  digitalWrite(binLF, LOW);
  analogWrite(pwmLF, pwm);

  digitalWrite(ainRF, HIGH);
  digitalWrite(binRF, LOW);
  analogWrite(pwmRF, pwm);
}

void moveBackward(int pwm) {
  digitalWrite(aInOne, LOW);
  digitalWrite(aInTwo, HIGH);
  analogWrite(pwmLB, pwm);

  digitalWrite(aInOneRB, LOW);
  digitalWrite(aInTwoRB, HIGH);
  analogWrite(pwmRB, pwm);

  digitalWrite(ainLF, LOW);
  digitalWrite(binLF, HIGH);
  analogWrite(pwmLF, pwm);

  digitalWrite(ainRF, LOW);
  digitalWrite(binRF, HIGH);
  analogWrite(pwmRF, pwm);
}

void strafeRight(int pwm) {
  digitalWrite(ainLF, HIGH);
  digitalWrite(binLF, LOW);
  analogWrite(pwmLF, pwm);

  digitalWrite(aInOne, LOW);
  digitalWrite(aInTwo, HIGH);
  analogWrite(pwmLB, pwm);

  digitalWrite(ainRF, LOW);
  digitalWrite(binRF, HIGH);
  analogWrite(pwmRF, pwm);

  digitalWrite(aInOneRB, HIGH);
  digitalWrite(aInTwoRB, LOW);
  analogWrite(pwmRB, pwm);
}

void strafeLeft(int pwm) {
  digitalWrite(ainLF, LOW);
  digitalWrite(binLF, HIGH);
  analogWrite(pwmLF, pwm);

  digitalWrite(aInOne, HIGH);
  digitalWrite(aInTwo, LOW);
  analogWrite(pwmLB, pwm);

  digitalWrite(ainRF, HIGH);
  digitalWrite(binRF, LOW);
  analogWrite(pwmRF, pwm);

  digitalWrite(aInOneRB, LOW);
  digitalWrite(aInTwoRB, HIGH);
  analogWrite(pwmRB, pwm);
}

void rotateCenterCL(int pwm) {
  digitalWrite(ainLF, HIGH);
  digitalWrite(binLF, LOW);
  analogWrite(pwmLF, pwm);

  digitalWrite(ainRF, LOW);
  digitalWrite(binRF, HIGH);
  analogWrite(pwmRF, pwm);

  digitalWrite(aInOne, HIGH);
  digitalWrite(aInTwo, LOW);
  analogWrite(pwmLB, pwm);

  digitalWrite(aInOneRB, LOW);
  digitalWrite(aInTwoRB, HIGH);
  analogWrite(pwmRB, pwm);
}

void rotateCenterCC(int pwm) {
  digitalWrite(ainLF, LOW);
  digitalWrite(binLF, HIGH);
  analogWrite(pwmLF, pwm);

  digitalWrite(ainRF, HIGH);
  digitalWrite(binRF, LOW);
  analogWrite(pwmRF, pwm);

  digitalWrite(aInOne, LOW);
  digitalWrite(aInTwo, HIGH);
  analogWrite(pwmLB, pwm);

  digitalWrite(aInOneRB, HIGH);
  digitalWrite(aInTwoRB, LOW);
  analogWrite(pwmRB, pwm);
}

void stopWheels() {
  digitalWrite(ainRF, LOW);
  digitalWrite(binRF, LOW);
  digitalWrite(ainLF, LOW);
  digitalWrite(binLF, LOW);
  digitalWrite(aInOneRB, LOW);
  digitalWrite(aInTwoRB, LOW);
  digitalWrite(aInOne, LOW);
  digitalWrite(aInTwo, LOW);

  analogWrite(pwmLB, 0);
  analogWrite(pwmRB, 0);
  analogWrite(pwmLF, 0);
  analogWrite(pwmRF, 0);
}

// =========================
// Read command from Pi
// Expected: lin,ang
// Example: 0.250,-0.400
// =========================
void readPiCommand() {
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    int commaIndex = msg.indexOf(',');
    if (commaIndex > 0) {
      String linStr = msg.substring(0, commaIndex);
      String angStr = msg.substring(commaIndex + 1);

      linCmd = linStr.toFloat();
      angCmd = angStr.toFloat();

      Serial.print("RX lin: ");
      Serial.print(linCmd, 3);
      Serial.print(" ang: ");
      Serial.println(angCmd, 3);
    }
  }
}

// =========================
// Safe drive logic
// =========================
void driveFromCommand(float lin, float ang, float frontCm, float sideCm) {
  bool frontValid = (frontCm > 0.0);
  bool sideValid = (sideCm > 0.0);

  // Hard front stop for forward movement
  if (frontValid && frontCm < FRONT_STOP_CM && lin > 0) {
    Serial.println("Front obstacle too close: STOP");
    stopWheels();
    return;
  }

  // Turning priority
  if (abs(ang) > ANG_DEADBAND) {
    int pwm = clampPWM((int)(TURN_PWM * min(abs(ang), 1.0f)));

    if (ang > 0) {
      rotateCenterCL(pwm);
    } else {
      rotateCenterCC(pwm);
    }
    return;
  }

  // Linear motion
  if (abs(lin) > LIN_DEADBAND) {
    float scale = min(abs(lin), 1.0f);

    // Slow down if getting somewhat close in front
    if (frontValid && lin > 0 && frontCm < FRONT_SLOW_CM) {
      scale *= 0.5;
      Serial.println("Front obstacle in slow zone");
    }

    int pwm = clampPWM((int)(BASE_PWM * scale));

    if (lin > 0) {
      moveForward(pwm);
    } else {
      moveBackward(pwm);
    }
    return;
  }

  stopWheels();

  if (sideValid && sideCm < SIDE_WARN_CM) {
    Serial.println("Side obstacle warning");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Phototransistor
  pinMode(ledPin, OUTPUT);
  thresholdADC = (thresholdVoltage / referenceVoltage) * adcResolution;

  // PWM pins
  pinMode(pwmLB, OUTPUT);
  pinMode(pwmRB, OUTPUT);
  pinMode(pwmLF, OUTPUT);
  pinMode(pwmRF, OUTPUT);

  // Direction pins
  pinMode(aInOne, OUTPUT);
  pinMode(aInTwo, OUTPUT);
  pinMode(aInOneRB, OUTPUT);
  pinMode(aInTwoRB, OUTPUT);
  pinMode(ainLF, OUTPUT);
  pinMode(binLF, OUTPUT);
  pinMode(ainRF, OUTPUT);
  pinMode(binRF, OUTPUT);

  // Ultrasonic
  pinMode(TRIG_PIN_F, OUTPUT);
  pinMode(ECHO_PIN_F, INPUT);
  pinMode(TRIG_PIN_S, OUTPUT);
  pinMode(ECHO_PIN_S, INPUT);

  // BNO055
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Starting BNO055...");
  if (!bno.begin()) {
    Serial.println("ERROR: BNO055 not detected.");
    while (1);
  }

  delay(1000);
  bno.setExtCrystalUse(true);

  Serial.println("BNO055 initialized!");
  stopWheels();
}

void loop() {
  // Wait for phototransistor trigger
  if (!robotStarted) {
    int sensorValue = analogRead(sensorPin);

    Serial.print("Sensor Value: ");
    Serial.print(sensorValue);
    Serial.print("  Threshold: ");
    Serial.println(thresholdADC);

    if (sensorValue > thresholdADC) {
      digitalWrite(ledPin, HIGH);
      robotStarted = true;
      Serial.println("Phototransistor triggered. Robot starting.");
      delay(300);
    } else {
      digitalWrite(ledPin, LOW);
      stopWheels();
      delay(100);
      return;
    }
  }

  // Read BNO055
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float yawDeg = euler.x();

  // Read Pi command
  readPiCommand();

  // Read ultrasonic sensors
  float frontCm = ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F);
  delay(10);
  float sideCm = ultrasonic_sensor(TRIG_PIN_S, ECHO_PIN_S);

  // Debug
  Serial.print("Yaw: ");
  Serial.print(yawDeg);
  Serial.print(" | linCmd: ");
  Serial.print(linCmd, 3);
  Serial.print(" | angCmd: ");
  Serial.print(angCmd, 3);
  Serial.print(" | Front(cm): ");
  Serial.print(frontCm);
  Serial.print(" | Side(cm): ");
  Serial.println(sideCm);

  // Drive with safety
  driveFromCommand(linCmd, angCmd, frontCm, sideCm);

  delay(30);
}