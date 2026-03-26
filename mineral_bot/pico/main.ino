#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Servo.h>
#include <utility/imumaths.h>
// final r5 code
/*
#if defined(ARDUINO_ARCH_RP2040)
#define DEBUG_SERIAL Serial
#define PI_SERIAL Serial1
#if !defined(ARDUINO_ARCH_MBED)
const int PI_UART_TX_PIN = 0; // GP0 -> RX de Raspberry Pi 5
const int PI_UART_RX_PIN = 1; // GP1 <- TX de Raspberry Pi 5
#endif
#else
#define DEBUG_SERIAL Serial
#define PI_SERIAL Serial
#endif
*/

// Comunicación por cable USB hacia la Raspberry Pi 5
#define DEBUG_SERIAL Serial
#define PI_SERIAL Serial

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
// BNO055
// =========================
#define SDA_PIN 4
#define SCL_PIN 5
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

// =========================
// Motor pins (Front, Back, Intake)
// =========================
// Front Left: InA=15 (PWM), InB=16 (DIR)
const int FrontLeftInA = 15;
const int FrontLeftInB = 16;
// Front Right: InA=14 (PWM), InB=13 (DIR)
const int FrontRightInA = 14;
const int FrontRightInB = 13;
// Back Left: InA=18 (PWM), InB=19 (DIR)
const int BackLeftInA = 18;
const int BackLeftInB = 19;
// Back Right: InA=21 (PWM), InB=20 (DIR)
const int BackRightInA = 21;
const int BackRightInB = 20;
// Intake: InA=22 (PWM), InB=23 (DIR)
const int IntakeInA = 22;
const int IntakeInB = 23;

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
const int sensorPin = 26; // ADC0 en Pico
const int ledPin = LED_BUILTIN;

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

enum ControlMode
{
  CONTROL_LEGACY_LIN_ANG,
  CONTROL_DIRECT_COMMAND
};

enum MotionCommand
{
  CMD_STOP,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_LEFT,
  CMD_RIGHT
};

ControlMode controlMode = CONTROL_DIRECT_COMMAND;
MotionCommand motionCmd = CMD_STOP;

int commandSpeed = 170;
bool mineralsEnabled = false;

float latestFrontCm = -1.0;
float latestSideCm = -1.0;
float latestYawDeg = 0.0;

// =========================
// Tunables
// =========================
const int BASE_PWM = 170;
const int TURN_PWM = 180;

const float LIN_DEADBAND = 0.08;
const float ANG_DEADBAND = 0.10;

// Ultrasonic thresholds
const float FRONT_STOP_CM = 25.0; // stop forward below this
const float FRONT_SLOW_CM = 45.0; // optional slow zone
const float SIDE_WARN_CM = 20.0;  // just debug for now

// =========================
// Helpers
// =========================
int clampPWM(int value)
{
  if (value < 0)
    return 0;
  if (value > 255)
    return 255;
  return value;
}

void applyMineralMotor()
{
  if (mineralsEnabled)
  {
    analogWrite(IntakeInA, commandSpeed);
    digitalWrite(IntakeInB, LOW);
  }
  else
  {
    analogWrite(IntakeInA, 0);
    digitalWrite(IntakeInB, LOW);
  }
}

// =========================
// Ultrasonic
// =========================
float ultrasonic_sensor(int TRIG_PIN, int ECHO_PIN)
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration_us = pulseIn(ECHO_PIN, HIGH, 30000); // 30 ms timeout

  if (duration_us == 0)
  {
    return -1.0; // invalid reading
  }

  float distance_cm = 0.017 * duration_us;
  return distance_cm;
}

// =========================
// Motor functions / finalmovemnt
// =========================
void moveForward(int pwm)
{
  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, LOW);
  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, LOW);
  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, LOW);
  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, LOW);
}

void moveBackward(int pwm)
{
  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, HIGH);
  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, HIGH);
  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, HIGH);
  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, HIGH);
}

void strafeRight(int pwm)
{
  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, LOW);
  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, HIGH);
  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, HIGH);
  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, LOW);
}

void strafeLeft(int pwm)
{
  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, HIGH);
  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, LOW);
  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, LOW);
  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, HIGH);
}

void rotateCenterCL(int pwm)
{
  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, LOW);
  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, HIGH);
  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, LOW);
  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, HIGH);
}

void rotateCenterCC(int pwm)
{
  analogWrite(FrontLeftInA, pwm);
  digitalWrite(FrontLeftInB, HIGH);
  analogWrite(FrontRightInA, pwm);
  digitalWrite(FrontRightInB, LOW);
  analogWrite(BackLeftInA, pwm);
  digitalWrite(BackLeftInB, HIGH);
  analogWrite(BackRightInA, pwm);
  digitalWrite(BackRightInB, LOW);
}

void stopWheels()
{
  analogWrite(FrontLeftInA, 0);
  digitalWrite(FrontLeftInB, LOW);
  analogWrite(FrontRightInA, 0);
  digitalWrite(FrontRightInB, LOW);
  analogWrite(BackLeftInA, 0);
  digitalWrite(BackLeftInB, LOW);
  analogWrite(BackRightInA, 0);
  digitalWrite(BackRightInB, LOW);
}

void stopAll()
{
  stopWheels();
  analogWrite(IntakeInA, 0);
  digitalWrite(IntakeInB, LOW);
}

// =========================
// Read command from Pi
// Supported direct commands:
// F, B, L, R, S, M1, M0, SPEED <0-255>, DIST, DATA, START
// Legacy compatibility:
// lin,ang  e.g. 0.250,-0.400
// =========================
void readPiCommand()
{
  while (PI_SERIAL.available())
  {
    String msg = PI_SERIAL.readStringUntil('\n');
    msg.trim();

    if (msg.length() == 0)
    {
      continue;
    }

    String msgUpper = msg;
    msgUpper.toUpperCase();

    int commaIndex = msg.indexOf(',');
    if (commaIndex > 0)
    {
      String linStr = msg.substring(0, commaIndex);
      String angStr = msg.substring(commaIndex + 1);

      linCmd = linStr.toFloat();
      angCmd = angStr.toFloat();
      controlMode = CONTROL_LEGACY_LIN_ANG;
      robotStarted = true;

      DEBUG_SERIAL.print("RX lin: ");
      DEBUG_SERIAL.print(linCmd, 3);
      DEBUG_SERIAL.print(" ang: ");
      DEBUG_SERIAL.println(angCmd, 3);
      continue;
    }

    controlMode = CONTROL_DIRECT_COMMAND;
    robotStarted = true;

    if (msgUpper == "F")
    {
      motionCmd = CMD_FORWARD;
    }
    else if (msgUpper == "B")
    {
      motionCmd = CMD_BACKWARD;
    }
    else if (msgUpper == "L")
    {
      motionCmd = CMD_LEFT;
    }
    else if (msgUpper == "R")
    {
      motionCmd = CMD_RIGHT;
    }
    else if (msgUpper == "S")
    {
      motionCmd = CMD_STOP;
    }
    else if (msgUpper == "M1")
    {
      mineralsEnabled = true;
      applyMineralMotor();
    }
    else if (msgUpper == "M0")
    {
      mineralsEnabled = false;
      applyMineralMotor();
    }
    else if (msgUpper.startsWith("SPEED"))
    {
      int spaceIndex = msgUpper.indexOf(' ');
      if (spaceIndex > 0 && spaceIndex < (msgUpper.length() - 1))
      {
        int parsedSpeed = msgUpper.substring(spaceIndex + 1).toInt();
        commandSpeed = clampPWM(parsedSpeed);
        applyMineralMotor();
      }
    }
    else if (msgUpper == "DIST")
    {
      float frontCm = ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F);
      latestFrontCm = frontCm;
      PI_SERIAL.print("DIST:");
      PI_SERIAL.println(frontCm, 2);
    }
    else if (msgUpper == "DATA")
    {
      PI_SERIAL.print("DATA:");
      PI_SERIAL.print(latestYawDeg, 2);
      PI_SERIAL.print(",");
      PI_SERIAL.print(latestFrontCm, 2);
      PI_SERIAL.print(",");
      PI_SERIAL.print(latestSideCm, 2);
      PI_SERIAL.print(",");
      PI_SERIAL.println(robotStarted ? 1 : 0);
    }
    else if (msgUpper == "START")
    {
      robotStarted = true;
    }
  }
}

void driveFromDirectCommand(float frontCm)
{
  bool frontValid = (frontCm > 0.0);
  int pwm = clampPWM(commandSpeed);

  if (motionCmd == CMD_FORWARD)
  {
    if (frontValid && frontCm < FRONT_STOP_CM)
    {
      stopWheels();
    }
    else
    {
      moveForward(pwm);
    }
  }
  else if (motionCmd == CMD_BACKWARD)
  {
    moveBackward(pwm);
  }
  else if (motionCmd == CMD_LEFT)
  {
    rotateCenterCC(pwm);
  }
  else if (motionCmd == CMD_RIGHT)
  {
    rotateCenterCL(pwm);
  }
  else
  {
    stopWheels();
  }

  applyMineralMotor();
}

// =========================
// Safe drive logic
// =========================
void driveFromCommand(float lin, float ang, float frontCm, float sideCm)
{
  bool frontValid = (frontCm > 0.0);
  bool sideValid = (sideCm > 0.0);

  // Hard front stop for forward movement
  if (frontValid && frontCm < FRONT_STOP_CM && lin > 0)
  {
    DEBUG_SERIAL.println("Front obstacle too close: STOP");
    stopWheels();
    return;
  }

  // Turning priority
  if (abs(ang) > ANG_DEADBAND)
  {
    int pwm = clampPWM((int)(TURN_PWM * min(abs(ang), 1.0f)));

    if (ang > 0)
    {
      rotateCenterCL(pwm);
    }
    else
    {
      rotateCenterCC(pwm);
    }
    return;
  }

  // Linear motion
  if (abs(lin) > LIN_DEADBAND)
  {
    float scale = min(abs(lin), 1.0f);

    // Slow down if getting somewhat close in front
    if (frontValid && lin > 0 && frontCm < FRONT_SLOW_CM)
    {
      scale *= 0.5;
      DEBUG_SERIAL.println("Front obstacle in slow zone");
    }

    int pwm = clampPWM((int)(BASE_PWM * scale));

    if (lin > 0)
    {
      moveForward(pwm);
    }
    else
    {
      moveBackward(pwm);
    }
    return;
  }

  stopWheels();

  if (sideValid && sideCm < SIDE_WARN_CM)
  {
    DEBUG_SERIAL.println("Side obstacle warning");
  }
}

void setup()
{
  DEBUG_SERIAL.begin(115200);
#if defined(ARDUINO_ARCH_RP2040)
#if !defined(ARDUINO_ARCH_MBED)
  PI_SERIAL.setTX(PI_UART_TX_PIN);
  PI_SERIAL.setRX(PI_UART_RX_PIN);
#endif
  PI_SERIAL.begin(115200);
#else
  PI_SERIAL.begin(115200);
#endif
  PI_SERIAL.setTimeout(25);
  delay(1000);

  // Phototransistor
  pinMode(ledPin, OUTPUT);
  thresholdADC = (thresholdVoltage / referenceVoltage) * adcResolution;

  // Motor pins
  pinMode(FrontLeftInA, OUTPUT);
  pinMode(FrontLeftInB, OUTPUT);
  pinMode(FrontRightInA, OUTPUT);
  pinMode(FrontRightInB, OUTPUT);
  pinMode(BackLeftInA, OUTPUT);
  pinMode(BackLeftInB, OUTPUT);
  pinMode(BackRightInA, OUTPUT);
  pinMode(BackRightInB, OUTPUT);
  pinMode(IntakeInA, OUTPUT);
  pinMode(IntakeInB, OUTPUT);

  // Servos
  myServoLeft.attach(servoLeft, 500, 2500);
  myServoRight.attach(servoRight, 500, 2500);
  myServoBeacon.attach(servoBeacon, 500, 2500);
  myServoLeft.write(0);
  delay(500);
  myServoRight.write(90);
  delay(500);
  myServoBeacon.write(0);
  delay(500);

  // Ultrasonic
  pinMode(TRIG_PIN_F, OUTPUT);
  pinMode(ECHO_PIN_F, INPUT);
  pinMode(TRIG_PIN_S, OUTPUT);
  pinMode(ECHO_PIN_S, INPUT);

  // BNO055
#if defined(ARDUINO_ARCH_MBED)
  Wire.begin();
#elif defined(ARDUINO_ARCH_RP2040)
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();
#else
  Wire.begin(SDA_PIN, SCL_PIN);
#endif

  DEBUG_SERIAL.println("Starting BNO055...");
  if (!bno.begin())
  {
    DEBUG_SERIAL.println("ERROR: BNO055 not detected.");
    while (1)
      ;
  }

  delay(1000);
  bno.setExtCrystalUse(true);

  DEBUG_SERIAL.println("BNO055 initialized!");
  stopAll();
}

void loop()
{
  // Wait for phototransistor trigger
  if (!robotStarted)
  {
    int sensorValue = analogRead(sensorPin);

    DEBUG_SERIAL.print("Sensor Value: ");
    DEBUG_SERIAL.print(sensorValue);
    DEBUG_SERIAL.print("  Threshold: ");
    DEBUG_SERIAL.println(thresholdADC);

    if (sensorValue > thresholdADC)
    {
      digitalWrite(ledPin, HIGH);
      robotStarted = true;
      DEBUG_SERIAL.println("Phototransistor triggered. Robot starting.");
      delay(300);
    }
    else
    {
      digitalWrite(ledPin, LOW);
      stopAll();
      delay(100);
      return;
    }
  }

  // Read BNO055
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float yawDeg = euler.x();
  latestYawDeg = yawDeg;

  // Read ultrasonic sensors
  float frontCm = ultrasonic_sensor(TRIG_PIN_F, ECHO_PIN_F);
  delay(10);
  float sideCm = ultrasonic_sensor(TRIG_PIN_S, ECHO_PIN_S);
  latestFrontCm = frontCm;
  latestSideCm = sideCm;

  // Read Pi command
  readPiCommand();

  // Debug
  DEBUG_SERIAL.print("Yaw: ");
  DEBUG_SERIAL.print(yawDeg);
  DEBUG_SERIAL.print(" | linCmd: ");
  DEBUG_SERIAL.print(linCmd, 3);
  DEBUG_SERIAL.print(" | angCmd: ");
  DEBUG_SERIAL.print(angCmd, 3);
  DEBUG_SERIAL.print(" | Front(cm): ");
  DEBUG_SERIAL.print(frontCm);
  DEBUG_SERIAL.print(" | Side(cm): ");
  DEBUG_SERIAL.println(sideCm);

  // Drive with safety
  if (controlMode == CONTROL_LEGACY_LIN_ANG)
  {
    driveFromCommand(linCmd, angCmd, frontCm, sideCm);
    applyMineralMotor();
  }
  else
  {
    driveFromDirectCommand(frontCm);
  }

  delay(30);
}