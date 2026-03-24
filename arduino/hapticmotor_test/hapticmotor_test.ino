#include <Wire.h>
#include <Adafruit_DRV2605.h>

Adafruit_DRV2605 drv;

#define I2C_SDA D4
#define I2C_SCL D5

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nDRV2605L test...");

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!drv.begin()) {
    Serial.println("ERROR: DRV2605 not found on I2C!");
    while (1) delay(10);
  }
  Serial.println("DRV2605 OK");

  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);
  drv.setWaveform(0, 1);  // effect 1
  drv.setWaveform(1, 0);  // end
}

void loop() {
  Serial.println("Buzz!");
  drv.go();
  delay(500);
}
