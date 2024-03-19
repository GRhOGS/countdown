#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();  // call pwm driver board at default adress 0x40

// Depending on your servo make, the pulse width min and max may vary, you 
// want these to be as small/large as possible without hitting the hard stop
// for max range. You'll have to tweak them as necessary to match the servos you
// have!
#define SERVOMIN  150 // This is the 'minimum' pulse length count (out of 4096), 150 is good default
#define SERVOMID  375 // should be center of servo
#define SERVOMAX  540 // This is the 'maximum' pulse length count (out of 4096), 600 is good default

#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define OSC_FREQ 25000000 // Set internal oscillator frequency. Normally: 25MHz, but realisticaly between 23-27MHz. Adjust this so that PWM pins on board deliver 50Hz signal

// our servo # counter
uint8_t servonum = 0;

// defines servo pulse widths for servo pointed away [0] and showing [1]
// 7 segment display positions 0-6: starting at the top, going clockwise, with middle segment being last element
uint16_t onesPositions[7][2] = {{SERVOMIN, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMIN+30, SERVOMID},
                                {SERVOMIN, SERVOMID},
                                {SERVOMAX-30, SERVOMID}};   // dont turn middle segment all the way, crashes into other servo otherwise
uint16_t tensPositions[7][2] = {{SERVOMIN, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMIN, SERVOMID},
                                {SERVOMIN, SERVOMID},
                                {SERVOMAX-30, SERVOMID}};   // dont turn middle segment all the way, crashes into other servo otherwise

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setOscillatorFrequency(OSC_FREQ);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);
}

void loop() {
  // use those two lines for manual testing of single servos
  //pwm.setPWM(9, 0, 540);
  //delay(99999999);

  // iterate through all servos
  // ones
  Serial.println("-------Ones--------");
  for (int servNum = 0; servNum < 7; servNum++){
    delay(500);
    pwm.setPWM(servNum, 0, onesPositions[servNum][1]);
  }
  for (int servNum = 6; servNum >= 0; servNum--){
    delay(500);
    pwm.setPWM(servNum, 0, onesPositions[servNum][0]);
  }
  // tens
  Serial.println("-------Tens--------");
  for (int servNum = 8; servNum < 15; servNum++){
    delay(500);
    pwm.setPWM(servNum, 0, tensPositions[servNum-8][1]);
  }
  for (int servNum = 14; servNum > 7; servNum--){
    delay(500);
    pwm.setPWM(servNum, 0, tensPositions[servNum-8][0]);
  }
  delay(1000);
}
