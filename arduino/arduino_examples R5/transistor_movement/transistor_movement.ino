const int pwmLB = 16;
const int pwmRB = 4;
const int pwmLF = 2;
const int pwmRF = 15;

// LB = BR
// RB = AR
// LF = AR
// RF = BR

const int aInOne = 5;
const int aInTwo = 17;
const int aInOneRB = 19;
const int aInTwoRB = 18;
const int ainLF = 14;
const int binLF = 27;   // changed from 17 to avoid pin conflict
const int ainRF = 12;
const int binRF = 13;

const int TRIG_PIN_S = 23;
const int ECHO_PIN_S = 26;
const int TRIG_PIN_F = 32;
const int ECHO_PIN_F = 36;

// Phototransistor
const int sensorPin = 34;
const int ledPin = 25;   // changed from 2 because 2 is already pwmLF

float thresholdVoltage = 0.2;   // higher = less sensitive
float referenceVoltage = 3.3;
int adcResolution = 4095;
int thresholdADC;

bool robotStarted = false;

void setup() {
  Serial.begin(115200);

  // phototransistor
  pinMode(ledPin, OUTPUT);
  thresholdADC = (thresholdVoltage / referenceVoltage) * adcResolution;

  // PWM pins
  pinMode(pwmLB, OUTPUT);
  pinMode(pwmRB, OUTPUT);
  pinMode(pwmLF, OUTPUT);
  pinMode(pwmRF, OUTPUT);

  // Back left pins
  pinMode(aInOne, OUTPUT);
  pinMode(aInTwo, OUTPUT);

  // Front left pins
  pinMode(ainLF, OUTPUT);
  pinMode(binLF, OUTPUT);

  // Back right pins
  pinMode(aInOneRB, OUTPUT);
  pinMode(aInTwoRB, OUTPUT);

  // Front right pins
  pinMode(ainRF, OUTPUT);
  pinMode(binRF, OUTPUT);

  // Ultrasonic
  pinMode(TRIG_PIN_F, OUTPUT);
  pinMode(ECHO_PIN_F, INPUT);
  pinMode(TRIG_PIN_S, OUTPUT);
  pinMode(ECHO_PIN_S, INPUT);

  stopWheels();
}

// Make sure to use delay(500); or some type of delay after calling function.
float ultrasonic_sensor(int TRIG_PIN, int ECHO_PIN) {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  float duration_us = pulseIn(ECHO_PIN, HIGH);
  float distance_cm = 0.017 * duration_us;

  Serial.print("Distance Measured: ");
  Serial.println(distance_cm);

  return distance_cm;
}

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
  // Front Left Move Forward
  digitalWrite(ainLF, HIGH);
  digitalWrite(binLF, LOW);
  analogWrite(pwmLF, pwm);

  // Back Left Move Backward
  digitalWrite(aInOne, LOW);
  digitalWrite(aInTwo, HIGH);
  analogWrite(pwmLB, pwm);

  // Front Right Move Backward
  digitalWrite(ainRF, LOW);
  digitalWrite(binRF, HIGH);
  analogWrite(pwmRF, pwm);

  // Back Right Move Forward
  digitalWrite(aInOneRB, HIGH);
  digitalWrite(aInTwoRB, LOW);
  analogWrite(pwmRB, pwm);
}

void strafeLeft(int pwm) {
  // Front Left Move Backward
  digitalWrite(ainLF, LOW);
  digitalWrite(binLF, HIGH);
  analogWrite(pwmLF, pwm);

  // Back Left Move Forward
  digitalWrite(aInOne, HIGH);
  digitalWrite(aInTwo, LOW);
  analogWrite(pwmLB, pwm);

  // Front Right Move Forward
  digitalWrite(ainRF, HIGH);
  digitalWrite(binRF, LOW);
  analogWrite(pwmRF, pwm);

  // Back Right Move Backward
  digitalWrite(aInOneRB, LOW);
  digitalWrite(aInTwoRB, HIGH);
  analogWrite(pwmRB, pwm);
}

void rotateCenterCL(int pwm) {
  // Front left move Forward
  digitalWrite(ainLF, HIGH);
  digitalWrite(binLF, LOW);
  analogWrite(pwmLF, pwm);

  // Front right move Backward
  digitalWrite(ainRF, LOW);
  digitalWrite(binRF, HIGH);
  analogWrite(pwmRF, pwm);

  // Back left move Forward
  digitalWrite(aInOne, HIGH);
  digitalWrite(aInTwo, LOW);
  analogWrite(pwmLB, pwm);

  // Back right move Backward
  digitalWrite(aInOneRB, LOW);
  digitalWrite(aInTwoRB, HIGH);
  analogWrite(pwmRB, pwm);
}

void rotateCenterCC(int pwm) {
  // Front left move Backward
  digitalWrite(ainLF, LOW);
  digitalWrite(binLF, HIGH);
  analogWrite(pwmLF, pwm);

  // Front right move Forward
  digitalWrite(ainRF, HIGH);
  digitalWrite(binRF, LOW);
  analogWrite(pwmRF, pwm);

  // Back left move Backward
  digitalWrite(aInOne, LOW);
  digitalWrite(aInTwo, HIGH);
  analogWrite(pwmLB, pwm);

  // Back right move Forward
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

void loop() {
  // Wait for phototransistor to trigger robot start
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

  // 1. Move forward until the ultrasonic sensor on front reads about 45 cm
  while (ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F) > 45) {
    moveForward(150);
  }

  stopWheels();
  delay(300);

  rotateCenterCL(200);
  delay(1500);
  stopWheels();
  delay(300);

  while (ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F) > 45) {
    moveForward(150);
  }

  stopWheels();
  delay(10000);

  // stop after one run
  while (1) {
    stopWheels();
    digitalWrite(ledPin, HIGH);
  }
}