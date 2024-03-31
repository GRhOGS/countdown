#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>    // adafruit servo driver lib
#include <RTClib.h>                     // adafruits rtc lib
#include <LowPower.h>                   // Low Power library from Rocket Scream Electronics

// 7 segment display positions 0-6: starting at the top, going clockwise, with middle segment being last element
// servos for the ones go into slots 0-6, servos for tens go into slots 8-14
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();  // call pwm driver board at default adress 0x40

// dervo min and max pulse length, depending on servo model, change to fit!
#define SERVO_MIN  150 // This is the 'minimum' pulse length count (out of 4096), 150 is good default
#define SERVO_MID  375 // should be center of servo
#define SERVO_MAX  540 // This is the 'maximum' pulse length count (out of 4096), 600 is good default

// configuration for servo driver board
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define OSC_FREQ 25000000 // Set internal oscillator frequency. Normally: 25MHz, but realisticaly between 23-27MHz. Adjust this so that PWM pins on board deliver 50Hz signal

// defines servo pulse widths for servo pointed away [0] and showing [1]
// use this matrix for fine adjustments so that the segments line up how you want them to
const uint16_t onesPositions[7][2] = {{SERVO_MIN, SERVO_MID},
                                      {SERVO_MAX, SERVO_MID-20},
                                      {SERVO_MAX, SERVO_MID+10},
                                      {SERVO_MAX, SERVO_MID-10},
                                      {SERVO_MIN+30, SERVO_MID+10},
                                      {SERVO_MIN, SERVO_MID+20},
                                      {SERVO_MAX, SERVO_MID}};
const uint16_t tensPositions[7][2] = {{SERVO_MIN, SERVO_MID+10},
                                      {SERVO_MAX, SERVO_MID},
                                      {SERVO_MAX-10, SERVO_MID},
                                      {SERVO_MAX, SERVO_MID-20},
                                      {SERVO_MIN, SERVO_MID},
                                      {SERVO_MIN, SERVO_MID},
                                      {SERVO_MAX-10, SERVO_MID}};

// contains which segments are switched up for which number
const int numberPositions[11][7] = { {1, 1, 1, 1, 1, 1, 0}, // 0
                                    {0, 1, 1, 0, 0, 0, 0},  // 1...
                                    {1, 1, 0, 1, 1, 0, 1},
                                    {1, 1, 1, 1, 0, 0, 1},
                                    {0, 1, 1, 0, 0, 1, 1},
                                    {1, 0, 1, 1, 0, 1, 1},
                                    {1, 0, 1, 1, 1, 1, 1},
                                    {1, 1, 1, 0, 0, 0, 0},
                                    {1, 1, 1, 1, 1, 1, 1},
                                    {1, 1, 1, 1, 0, 1, 1},
                                    {0, 0, 1, 0, 1, 1, 1}}; // "h" for "hi" on startup

// keep track of middle segment to avoid collisions
int onesMiddleShown;
int tensMiddleShown;

// time related inits
#define INT_PIN 2
RTC_PCF8523 rtc;
int16_t daysToGo = 800;  // >2 years, ensures "new day" is triggered on first loop() cycle
volatile bool interruptFlag = false;
DateTime targetDate;

void wakeyWakey(){}     // interrupt function

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

void displayDaysLeft(int16_t num){
  if(num < 0){
    num = 0;
  }
  if(num > 99){
    num = 99;
  }
  pwm.wakeup();
  setOnes(num%10);
  setTens(num/10);
  delay(2000);
  pwm.sleep();
}

void setup() {
  Serial.begin(9600);
  // Setup servo controller
  Serial.println("Setting up servo controller...");
  pwm.begin();
  pwm.setOscillatorFrequency(OSC_FREQ);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);
  // hi message
  tensMiddleShown = 0;
  setTens(10);
  onesMiddleShown = 1;
  setOnes(1);

  // setup RTC
  //rtc.start();
  Serial.println("Setting up RTC...");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (!rtc.initialized() || rtc.lostPower()){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // sets RTC to time this sketch was compiled
  }
  rtc.deconfigureAllTimers();   // Timer configuration is not cleared on an RTC reset due to battery backup!
  pinMode(INT_PIN, INPUT_PULLUP);
  
  // get first reading
  DateTime now = rtc.now();
  Serial.print("Right now the date and time are: ");
  Serial.print(now.day()); Serial.print(".");
  Serial.print(now.month()); Serial.print(".");
  Serial.print(now.year()); Serial.print(" ");
  Serial.print(now.hour()); Serial.print(":");
  Serial.println(now.minute());
  
  Serial.println("Reading target date...");
  // setup input pins for date selection
  const uint8_t DAY_PINS[5] = {8, 9, 10, 11, 12};   // day
  const uint8_t MONTH_PINS[4] = {3, 4, 5, 6};       // month
  // hour, year
  const uint8_t HOUR_PIN_1 = A1;
  const uint8_t HOUR_PIN_2 = A0;
  const uint8_t YEAR_PIN = A3;

  //setup pinmodes
  for(int i=0; i<=4; i++){
    pinMode(DAY_PINS[i], INPUT_PULLUP);
    if(i < 4) pinMode(MONTH_PINS[i], INPUT_PULLUP);
  }
  pinMode(HOUR_PIN_1, INPUT_PULLUP);
  pinMode(HOUR_PIN_2, INPUT_PULLUP);
  pinMode(YEAR_PIN, INPUT_PULLUP);

  // convvert selected date from binary to int, save in variables
  int day = 0;
  int month = 0;
  for(int i = 0; i <= 4; i++){
    day += !digitalRead(DAY_PINS[i])<<i;                 // day is 5 bit long
    if(i < 4) month += !digitalRead(MONTH_PINS[i])<<i;   // month is 4 bit long
  }

  uint8_t hour = 1 * !digitalRead(HOUR_PIN_1) + 2 * !digitalRead(HOUR_PIN_2);
  uint16_t year = !digitalRead(YEAR_PIN);

  // set according target year to this or next year depending on year input (shorted pin = even year)
  if((year == 1 && now.year() % 2 == 0) or (year == 0 and now.year() % 2 == 1)){
    year = now.year();
  } else {
    year = now.year() + 1;
  }
  // "round" selection to valid date
  if(month == 0) month = 1;
  if(month > 12) month = 12;
  if(day == 0) day == 1;
  if(month == 2 && day > 28){         // adjust max day in february
    if(year % 4 == 0) day = 29;       // leap year
    else day = 28;                    // no leap year
    Serial.print("Rounded day to ");
    Serial.print(day);
    Serial.println(" because month is set to February!");
  }
  if(month % 2 == 1 && day > 30){
    day = 30;
    Serial.println("Uneven month only last 30 day, rounded '31' to '30'!");
  }

  // convert hour selection to real hour value
  if(hour == 0) hour = 0;
  else if(hour == 1) hour = 9;
  else if(hour == 2) hour = 15;
  else hour = 21;
  
  // save for use in main loop
  targetDate = DateTime(year, month, day, hour, 0, 0); // year, month, day, hour, minute, second

  // print target date
  Serial.print("target date: ");
  Serial.print(targetDate.day());
  Serial.print(".");
  Serial.print(targetDate.month());
  Serial.print(".");
  Serial.print(targetDate.year());
  Serial.print(", hour: ");
  Serial.println(targetDate.hour());

  // trigger first rtc interrupt, so that subsequent interrupts are accurate
  rtc.enableCountdownTimer(PCF8523_Frequency64Hz, 1);     // set short countdown
  delay(50);                                              // wait for alarm to trigger
  rtc.deconfigureAllTimers();                             // turn off alarm
  delay(2000); // let hi message linger a little
}

void loop() {
  rtc.deconfigureAllTimers();     // turn off alarm set at the end of loop()
  DateTime now = rtc.now();
  TimeSpan timeLeft = targetDate - now;
  Serial.print("time left: ");
  Serial.print(timeLeft.days());
  Serial.print(" days ");
  Serial.print(timeLeft.hours());
  Serial.print(" hours ");
  Serial.print(timeLeft.minutes());
  Serial.println(" minutes.");

  // on new day
  if(timeLeft.days() < daysToGo){   //timeLeft was just calculated, daysToGo is used as comparison!
    if(timeLeft.days() < 0) daysToGo = 0;    // catch case when target date has been passed
    else daysToGo = timeLeft.days();         // update date
    int offset = int((timeLeft.hours() > 0) && (timeLeft.minutes() > 0));
    displayDaysLeft(daysToGo + offset);      // display days left
    if(daysToGo == 0){                       // terminate program when goal is reached
      Serial.println("Target date reached, please reset with new date :)");
      while(1) delay(10);
    }
  }

  // set timer
  Serial.print("Going to sleep for ");
  if(timeLeft.hours() > 0){
    rtc.enableCountdownTimer(PCF8523_FrequencyHour, timeLeft.hours());  // >1h remaining
    Serial.print(timeLeft.hours()); Serial.print(" hours.");
  } else {
    rtc.enableCountdownTimer(PCF8523_FrequencyMinute, timeLeft.minutes() + 1);  // <1h remaining, +1 makes sure final hour is definately passed
    Serial.print(timeLeft.minutes()+1); Serial.print(" minutes.");
  }
  Serial.println(" Sleep tight :)");

  attachInterrupt(digitalPinToInterrupt(INT_PIN), wakeyWakey, LOW);                           // set interrupt
  delay(100);                                                          // wait a bit for processes to finish before sleeping
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                 // go to sleep
  detachInterrupt(digitalPinToInterrupt(INT_PIN));                                            // got woken up by alarm
  Serial.println("woke up!");
}
