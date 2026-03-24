/**********************************************
 esp32s ultrasonic test
  here were going to use the esp primarily for sensor/signal processing
  This program has 1 purpose 
      -to read distances
      -print readings
  IMPORTANT
  -alot of ultrasonic sensors output a 5v signal though ECHO (esp needs 3.3v max)
  -use a voltage dividor on ECHO (im assuming out module is the 5v version)
**********************************************/
#include <arduino.h>

const int TRIG_PIN = 25;
const int ECHO_PIN = 26;

float ustoCm (long us){
// speed of sound is 343 m/s which is 0.0343cm/us
// distance (cm) = (echo_time_us * 0.0343cm/us)
return (us * 0.0343f) / 2.0f;
}

// we want a raw measurment (if NOTHING then return -1)

float readULTSONICcm(){
  // this is a clean tigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  //10us HIGH pulse triggers measurment
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  //read echo pulse width(this is my freesing prevention/timoeout)
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // A 30MS TIMEMOUT (like 5 m or somthing)

  if (duration == 0 ) return -1.0f; //nada. no echo
  return ustoCm(duration);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("ultrasonic test started...");
}

float median3(float a, float b, float c){ //just decides median thats it.... {
  if (a > b) { float t=a; a=b; b=t; }
  if (b > c) { float t=b; b=c; c=t; }
  if (a > b) { float t=a; a=b; b=t; }
  return b;
}


void loop() {
  
  float d1 = readULTSONICcm();
  delay(20);
  float d2 = readULTSONICcm();
  delay(20);
  float d3 = readULTSONICcm();

  float d = median3( d1, d2, d3);

  if(d<0){
    Serial.println("Distance: (no ECHO)");
  }
  else{
    Serial.print("Distance: ");
    Serial.print(d, 1);
    Serial.println(" cm");
  }
  
  delay(100); // 10hz



}
