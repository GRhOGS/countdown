#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 7 segment display positions 0-6: starting at the top, going clockwise, with middle segment being last element
// servos for the ones go into slots 0-6, servos for tens go into slots 8-14
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

// defines servo pulse widths for servo pointed away [0] and showing [1]
uint16_t onesPositions[7][2] = {{SERVOMIN, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMIN+30, SERVOMID},
                                {SERVOMIN, SERVOMID},
                                {SERVOMAX-40, SERVOMID}};   // dont turn middle segment all the way, crashes into other servo otherwise
uint16_t tensPositions[7][2] = {{SERVOMIN, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMAX-10, SERVOMID},
                                {SERVOMAX, SERVOMID},
                                {SERVOMIN, SERVOMID},
                                {SERVOMIN, SERVOMID},
                                {SERVOMAX-40, SERVOMID}};   // dont turn middle segment all the way, crashes into other servo otherwise

// contains which segments are switched up for which number
int numberPositions[10][7] = { {1, 1, 1, 1, 1, 1, 0}, // 0
                              {0, 1, 1, 0, 0, 0, 0}, // 1...
                              {1, 1, 0, 1, 1, 0, 1},
                              {1, 1, 1, 1, 0, 0, 1},
                              {0, 1, 1, 0, 0, 1, 1},
                              {1, 0, 1, 1, 0, 1, 1},
                              {1, 0, 1, 1, 1, 1, 1},
                              {1, 1, 1, 0, 0, 0, 0},
                              {1, 1, 1, 1, 1, 1, 1},
                              {1, 1, 1, 1, 0, 1, 1}};

// keep track of middle segment to avoid collisions
int onesMiddleShown = 0;
int tensMiddleShown = 0;

void setOnes(int num){
  //middle segment
  int position = numberPositions[num][6];
  if(position != onesMiddleShown){
    // make space for middle segment to turn
    pwm.setPWM(2, 0, onesPositions[2][0]);
    pwm.setPWM(4, 0, onesPositions[4][0]);
    delay(100);
    // turn middle segment
    int angle = onesPositions[6][position];
    pwm.setPWM(6, 0, angle);
    delay(100);

    onesMiddleShown = position;
  }

  // other segments
  for (int servNum = 0; servNum < 6; servNum++){
    int position = numberPositions[num][servNum];
    int angle = onesPositions[servNum][position];
    pwm.setPWM(servNum, 0, angle);
    delay(50);
  }

}

void setTens(int num){
  //middle segment
  int position = numberPositions[num][6];
  if(position != tensMiddleShown){
    // make space for middle segment to turn
    pwm.setPWM(10, 0, tensPositions[2][0]);
    pwm.setPWM(12, 0, tensPositions[4][0]);
    delay(100);
    // turn middle segment
    int angle = tensPositions[6][position];
    pwm.setPWM(14, 0, angle);
    delay(100);

    tensMiddleShown = position;
  }
  
  // other segments
  for (int servNum = 8; servNum < 14; servNum++){
    int position = numberPositions[num][servNum-8];
    int angle = tensPositions[servNum-8][position];
    pwm.setPWM(servNum, 0, angle);
    delay(50);
  }
    
}

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setOscillatorFrequency(OSC_FREQ);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);
  // initialize all segments set shown
  setOnes(8);
  setTens(8);
}

void loop() {
  for(int i = 0; i < 10; i++){
    setOnes(i);
    setTens(i);
    delay(2000);
  }
}
