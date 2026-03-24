#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// ===== ESP32 I2C pins =====
// Change these if your wiring is different
#define SDA_PIN 21
#define SCL_PIN 22
//first
// Create sensor object
// First argument = sensor ID
// Second argument = I2C address (usually 0x28, sometimes 0x29)
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("BNO055 test starting...");

  if (!bno.begin()) {
    Serial.println("ERROR: BNO055 not detected.");
    Serial.println("Check wiring, power, and I2C address.");
    while (1)
      ;
  }

  delay(1000);

  // Use external crystal if your breakout supports it
  bno.setExtCrystalUse(true);

  Serial.println("BNO055 initialized!");
  Serial.println("Move sensor around slowly to improve calibration.");
}

void loop() {
  // Get orientation in Euler angles
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);

  // Get linear acceleration (gravity removed)
  imu::Vector<3> linearAccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

  // Get raw acceleration
  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  // Calibration status: 0 = uncalibrated, 3 = fully calibrated
  uint8_t system, gyro, accelCal, mag;
  bno.getCalibration(&system, &gyro, &accelCal, &mag);

  Serial.println("========== BNO055 ==========");
  Serial.print("Yaw:   ");
  Serial.print(euler.x());
  Serial.println(" deg");
  Serial.print("Roll:  ");
  Serial.print(euler.z());
  Serial.println(" deg");
  Serial.print("Pitch: ");
  Serial.print(euler.y());
  Serial.println(" deg");

  Serial.print("Lin Acc X: ");
  Serial.print(linearAccel.x());
  Serial.print("  ");
  Serial.print("Y: ");
  Serial.print(linearAccel.y());
  Serial.print("  ");
  Serial.print("Z: ");
  Serial.println(linearAccel.z());

  Serial.print("Accel X: ");
  Serial.print(accel.x());
  Serial.print("  ");
  Serial.print("Y: ");
  Serial.print(accel.y());
  Serial.print("  ");
  Serial.print("Z: ");
  Serial.println(accel.z());

  Serial.print("Calibration -> SYS: ");
  Serial.print(system);
  Serial.print(" G: ");
  Serial.print(gyro);
  Serial.print(" A: ");
  Serial.print(accelCal);
  Serial.print(" M: ");
  Serial.println(mag);

  delay(500);
}