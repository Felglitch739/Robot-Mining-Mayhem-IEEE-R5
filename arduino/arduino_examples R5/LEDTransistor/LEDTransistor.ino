const int sensorPin = 34;
const int ledPin = 2;

float thresholdVoltage = 0.2;   // higher = less sensitive
float referenceVoltage = 3.3;
int adcResolution = 4095;

int thresholdADC;

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  thresholdADC = (thresholdVoltage / referenceVoltage) * adcResolution;
}

void loop() {
  int sensorValue = analogRead(sensorPin);

  Serial.print("Sensor Value: ");
  Serial.print(sensorValue);
  Serial.print("  Threshold: ");
  Serial.println(thresholdADC);

  if (sensorValue > thresholdADC) {   // or > reverse bias 
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }

  delay(100);
}