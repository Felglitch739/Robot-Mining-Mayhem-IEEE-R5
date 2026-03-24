const int pwmLB = 16;
const int pwmRB = 4;
const int pwmLF = 2;
const int pwmRF = 15;
// LB = BR whats this for ??
// RB = AR
// LF = AR
// RF = BR
const int aInOne = 5;
const int aInTwo = 17;
const int aInOneRB = 19;
const int aInTwoRB = 18;
const int ainLF = 14;
const int binLF = 27;   // changed from 17 to avoid conflict
const int ainRF = 12;
const int binRF = 13;
const int TRIG_PIN_S = 23;
const int ECHO_PIN_S = 26;
const int TRIG_PIN_F = 32;
const int ECHO_PIN_F = 36;
const int sensorPin = 34; // phototransistor pin
const int ledPin = 2;     // LED

float threshold_Voltage = 0.5;
float refrence_Voltage = 3.3;

int ADC_Resolution = 4095; // 12-bit ADC
int threshold_ADC;

void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);

  threshold_ADC = (threshold_Voltage / refrence_Voltage) * ADC_Resolution;

  pinMode(pwmLB, OUTPUT);
  pinMode(pwmRB, OUTPUT);
  pinMode(pwmLF, OUTPUT);
  pinMode(pwmRF, OUTPUT);

  pinMode(aInOne, OUTPUT);
  pinMode(aInTwo, OUTPUT);

  pinMode(ainLF, OUTPUT);
  pinMode(binLF, OUTPUT);

  pinMode(aInOneRB, OUTPUT);
  pinMode(aInTwoRB, OUTPUT);

  pinMode(ainRF, OUTPUT);
  pinMode(binRF, OUTPUT);

  pinMode(TRIG_PIN_F, OUTPUT);
  pinMode(ECHO_PIN_F, INPUT);
  pinMode(TRIG_PIN_S, OUTPUT);
  pinMode(ECHO_PIN_S, INPUT);
}

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

  digitalWrite(aInOneRB, LOW);   // fixed
  digitalWrite(aInTwoRB, HIGH);  // fixed
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
}

void loop() {
  int sensorValue = analogRead(sensorPin);

  if (sensorValue > threshold_ADC) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }

  while (ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F) > 45) {
    moveForward(150);
  }

  stopWheels();
  rotateCenterCL(200);
  delay(1500);
  stopWheels();

  while (ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F) > 45) {
    moveForward(150);
  }

  stopWheels();
  delay(10000);
}