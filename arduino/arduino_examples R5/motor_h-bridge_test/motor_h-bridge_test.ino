//this is me guessing that everything on the motor driver is good, and the motors are set up and ready for testing...
/*****************************
okay this code has a simple purpose:
verify... 
      The arduino Uno is working and the H-bridge is wird correctly 
      The motors can spin forward and backward
      pwr and gnd are correct igz...?
====================================
this is simply test code  
we dont care about 
-serial 
-raspy pi
-esp 32
====================================
hopefully it works like this
  motor spins foward for like 2 seconds
  motors stop for a second
  motor spins backward for 2 more seconds
  motors stop for 2 seconds 
  goes forever....
===================================
pins 
  all motors need
    2 directions pins (IN1 / IN2)
    1 PWR pin( this can be either ENA or ENB) this is for speed

=================================
keep this in mind
  PWM pins have to be with a "~" on the uno (look carfully youll see it)
  ENA/ENB jumpers on the H-bridge NEED to be removed.
  (for the code ill try to  be in depth)
********************************/
// left motor (a)
const int ENA = 5;// PWM pin controls speed 
const int IN1 = 7;// Direction pin 1 
const int IN2 = 8;// Direction pin 2 

// right motor (b)
const int ENB = 6; // PWM pin controls speed 
const int IN3 = 9; // Direction pin 1 
const int IN4 = 10;// Direction pin 2 
 
 /**************************************
 Setup() runs once when the arduino powers on or resets 
  This here is where we
    -tell the arduino which pins are outputs
    -put the robot in a safe stopped state
 **************************************/

void setup() {
  // lets tell the arduino that these pins send signals OUT
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  //gotta do this.
  // this ensures that there is no background noise to the pins and we dont have them (the pins) acting werid...
  stopMotors();
}


/**************************************
Loop() runs FOREVER after setup() finishes
this is where we can test our motors
**************************************/
void loop() {
  //gonna drive forward
  //TECHNICAL: speed range is 0-255 (PWM duty cycle)
  driveForward(150);
  delay(2000); //lets keep it moving for 2 seconds

  //stop.
  stopMotors();
  delay(1000); //1 second pause

  //drive backwards
  driveBackwards(150);
  delay(2000);// i trust you alr know

  //stop again.
  stopMotors();
  delay(2000);
}

/**************************************
These are our motor control functions 
  ...essentially they turn whatever we want into pin signals...
  this is the logic we'll be following
  - IN1 HIGH + IN2 LOW => motor will spin a direction
  - IN1 LOW + IN2 HIGH => motor spins opposite direction 
  - Both LOW           => motos coast (free spins!)

Note: PWM controls SPEED, not DIRECTION...
**************************************/
// going forward!
void driveForward(int speed){
  //wanna make sure speed stays in a valid PWM range 
  speed = constrain(speed, 0 , 255);

  // Motor a
  digitalWrite(IN1,HIGH); // direction 
  digitalWrite(IN2,LOW);
  digitalWrite(ENA,speed); // speed

  // Motor b
  digitalWrite(IN3,HIGH); // direction
  digitalWrite(IN4,LOW);
  digitalWrite(ENB,speed); // speed
}

// going backward!
void driveBackwards(int speed){
  speed = constrain(speed, 0 , 255);

    // Motor a
  digitalWrite(IN1,HIGH); // direction 
  digitalWrite(IN2,LOW);
  digitalWrite(ENA,speed); // speed

  // Motor b
  digitalWrite(IN3,HIGH); // direction
  digitalWrite(IN4,LOW);
  digitalWrite(ENB,speed); // speed

}

//stop...

void stopMotors(){
  // setting speed to 0
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  //set direction pins LOW for coasting
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN4,LOW);

}

//congrats you know how to move the motors via arduino uno, 
//im kinda sure that we should use the same logic for the h-bridge but yeah, should be good

//tl;dr
// direction =>controlled by IN pins
// speed     => PWM (ENA/ENB)
// analogWrite() turns the pin off and on rlly fast (changes duty cycle)
// digitalWrite() only off or on (no speed control)