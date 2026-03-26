#include <Servo.h>

// =========================
// Servos
// =========================
Servo myServoLeft;
Servo myServoRight;
Servo myServoBeacon;

const int servoLeft = 12;
const int servoRight = 11;
const int servoBeacon = 10;

// =========================
// Motor pins (as requested)
// =========================
const int FrontLeftInA = 15;
const int FrontLeftInB = 16;
const int FrontRightInA = 14;
const int FrontRightInB = 13;
const int BackLeftInA = 18;
const int BackLeftInB = 19;
const int BackRightInA = 21;
const int BackRightInB = 20;

// Intake pins per your teammate test wiring
const int IntakeInA = 1;
const int IntakeInB = 2;

// =========================
// Ultrasonic pins
// =========================
const int TRIG_PIN_F = 6;
const int ECHO_PIN_F = 7;
const int TRIG_PIN_S = 8;
const int ECHO_PIN_S = 9;

// =========================
// Phototransistor
// =========================
const int sensorPin = 26; // ADC0
const int ledPin = LED_BUILTIN;

float thresholdVoltage = 0.5;
float referenceVoltage = 3.3;
int adcResolution = 4095;
int thresholdADC;

bool triggered = false;

// =========================
// Test parameters
// =========================
const int DRIVE_PWM = 150;
const float FRONT_STOP_CM = 30.0;
const unsigned long ULTRASONIC_TIMEOUT_US = 30000; // 30 ms

enum TestState
{
  WAIT_LIGHT,
  SERVO_TEST,
  SENSOR_TEST,
  MOTOR_TEST,
  AUTO_TEST
};

TestState state = WAIT_LIGHT;
unsigned long stateStartMs = 0;

// =========================
// Helpers
// =========================
int clampPWM(int value)
{
  if (value < 0) return 0;
  if (value > 255) return 255;
  return value;
}

void stopMotors()
{
  analogWrite(FrontLeftInA, 0);
  digitalWrite(FrontLeftInB, LOW);

  analogWrite(FrontRightInA, 0);
  digitalWrite(FrontRightInB, LOW);

  analogWrite(BackLeftInA, 0);
  digitalWrite(BackLeftInB, LOW);

  analogWrite(BackRightInA, 0);
  digitalWrite(BackRightInB, LOW);

  digitalWrite(IntakeInA, LOW);
  digitalWrite(IntakeInB, LOW);
}

void intakeOn()
{
  digitalWrite(IntakeInA, HIGH);
  digitalWrite(IntakeInB, LOW);
}

void intakeOff()
{
  digitalWrite(IntakeInA, LOW);
  digitalWrite(IntakeInB, LOW);
}

void moveForward(int pwm)
{
  pwm = clampPWM(pwm);

  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, LOW);

  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, LOW);

  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, LOW);

  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, LOW);

  intakeOn();
}

void moveBackward(int pwm)
{
  pwm = clampPWM(pwm);

  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, HIGH);

  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, HIGH);

  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, HIGH);

  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, HIGH);

  intakeOff();
}

void rotateLeft(int pwm)
{
  pwm = clampPWM(pwm);

  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, HIGH);

  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, LOW);

  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, HIGH);

  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, LOW);

  intakeOff();
}

void rotateRight(int pwm)
{
  pwm = clampPWM(pwm);

  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, LOW);

  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, HIGH);

  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, LOW);

  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, HIGH);

  intakeOff();
}

float ultrasonic_sensor(int TRIG_PIN, int ECHO_PIN)
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration_us = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration_us == 0)
  {
    return -1.0; // invalid/no echo
  }

  return 0.017f * duration_us;
}

void setState(TestState newState)
{
  state = newState;
  stateStartMs = millis();
}

// =========================
// Tests
// =========================
void runServoTest()
{
  Serial.println("[TEST] Servo sweep start");

  for (int angle = 0; angle <= 90; angle++)
  {
    myServoLeft.write(angle);
    myServoBeacon.write(angle);
    myServoRight.write(90 - angle);
    delay(12);
  }

  for (int angle = 90; angle >= 0; angle--)
  {
    myServoLeft.write(angle);
    myServoBeacon.write(angle);
    myServoRight.write(90 - angle);
    delay(12);
  }

  myServoLeft.write(0);
  myServoRight.write(90);
  myServoBeacon.write(0);

  Serial.println("[TEST] Servo sweep done");
  setState(SENSOR_TEST);
}

void runSensorTest()
{
  float frontCm = ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F);
  delay(8);
  float sideCm = ultrasonic_sensor(TRIG_PIN_S, ECHO_PIN_S);

  Serial.print("[TEST] Front(cm): ");
  Serial.print(frontCm, 2);
  Serial.print(" | Side(cm): ");
  Serial.println(sideCm, 2);

  if (millis() - stateStartMs > 5000)
  {
    Serial.println("[TEST] Ultrasonic test done");
    setState(MOTOR_TEST);
  }

  delay(120);
}

void runMotorTest()
{
  unsigned long elapsed = millis() - stateStartMs;

  if (elapsed < 2000)
  {
    moveForward(DRIVE_PWM);
  }
  else if (elapsed < 3000)
  {
    moveBackward(DRIVE_PWM);
  }
  else if (elapsed < 4000)
  {
    rotateLeft(DRIVE_PWM);
  }
  else if (elapsed < 5000)
  {
    rotateRight(DRIVE_PWM);
  }
  else if (elapsed < 6500)
  {
    stopMotors();
    intakeOn(); // intake-only test
  }
  else
  {
    stopMotors();
    Serial.println("[TEST] Motor + intake test done");
    setState(AUTO_TEST);
  }
}

void runAutoTest()
{
  float frontCm = ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F);
  delay(8);
  float sideCm = ultrasonic_sensor(TRIG_PIN_S, ECHO_PIN_S);

  Serial.print("[AUTO] Front(cm): ");
  Serial.print(frontCm, 2);
  Serial.print(" | Side(cm): ");
  Serial.println(sideCm, 2);

  bool frontValid = frontCm > 0.0;
  if (frontValid && frontCm < FRONT_STOP_CM)
  {
    stopMotors();
    delay(120);
    rotateRight(DRIVE_PWM);
    delay(350);
    stopMotors();
  }
  else
  {
    moveForward(DRIVE_PWM);
  }

  delay(40);
}

// =========================
// Setup / Loop
// =========================
void setup()
{
  Serial.begin(115200);
  delay(400);

  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);

  pinMode(FrontLeftInA, OUTPUT);
  pinMode(FrontLeftInB, OUTPUT);
  pinMode(FrontRightInA, OUTPUT);
  pinMode(FrontRightInB, OUTPUT);
  pinMode(BackLeftInA, OUTPUT);
  pinMode(BackLeftInB, OUTPUT);
  pinMode(BackRightInA, OUTPUT);
  pinMode(BackRightInB, OUTPUT);

  pinMode(TRIG_PIN_F, OUTPUT);
  pinMode(ECHO_PIN_F, INPUT);
  pinMode(TRIG_PIN_S, OUTPUT);
  pinMode(ECHO_PIN_S, INPUT);

  pinMode(IntakeInA, OUTPUT);
  pinMode(IntakeInB, OUTPUT);

  myServoLeft.attach(servoLeft, 500, 2500);
  myServoRight.attach(servoRight, 500, 2500);
  myServoBeacon.attach(servoBeacon, 500, 2500);

  myServoLeft.write(0);
  delay(300);
  myServoRight.write(90);
  delay(300);
  myServoBeacon.write(0);
  delay(300);

  thresholdADC = (int)((thresholdVoltage / referenceVoltage) * adcResolution);

  stopMotors();

  Serial.println("=== Pico standalone test mode ===");
  Serial.print("Phototransistor threshold ADC: ");
  Serial.println(thresholdADC);
  Serial.println("Waiting for start light...");
}

void loop()
{
  if (!triggered)
  {
    int sensorValue = analogRead(sensorPin);

    Serial.print("[WAIT_LIGHT] sensor=");
    Serial.print(sensorValue);
    Serial.print(" threshold=");
    Serial.println(thresholdADC);

    if (sensorValue > thresholdADC)
    {
      triggered = true;
      digitalWrite(ledPin, HIGH);
      Serial.println("Start light detected. Running test sequence...");
      setState(SERVO_TEST);
      delay(200);
    }
    else
    {
      digitalWrite(ledPin, LOW);
      stopMotors();
      delay(120);
      return;
    }
  }

  switch (state)
  {
    case SERVO_TEST:
      runServoTest();
      break;

    case SENSOR_TEST:
      runSensorTest();
      break;

    case MOTOR_TEST:
      runMotorTest();
      break;

    case AUTO_TEST:
      runAutoTest();
      break;

    case WAIT_LIGHT:
    default:
      break;
  }
}