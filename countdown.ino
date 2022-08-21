#include <RTClib.h>                     // adafruits rtc lib
#include <Adafruit_PWMServoDriver.h>    // adafruits servo driver lib
#include <LowPower.h>                   // Rocket Scream Electronics lib for sleeping 
#include <Wire.h>

// global objects and variables
#define INT_PIN 2
#define SERVO_MIN  150    // This is the 'minimum' pulse length count (out of 4096)
#define SERVO_MAX  600    // This is the 'maximum' pulse length count (out of 4096)
uint16_t daysToGo = 800;  // >2 years, ensures "new day" is triggered on first loop() cycle
volatile bool interruptFlag = false;
DateTime targetDate;
RTC_PCF8523 rtc;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();    // servo driver on I2C address 0x40
uint16_t onesNeutralState[7] = {375, 375, 375, 375, 375, 375, 375};
uint16_t tensNeutralState[7] = {375, 375, 375, 375, 375, 375, 375};
//                               l,   l,   r,   r,   r,   l,   r  
uint16_t onesTurnedState[7] = {375, 375, 375, 375, 375, 375, 375}; 
uint16_t tensTurnedState[7] = {375, 375, 375, 375, 375, 375, 375}; 


void terminateProgram(){
  // pulsate led for 10min
  long oldTime = millis();
  while (oldTime + (100*60*10) > millis()){
    for (uint16_t i=1024; i<=4096; i++){
      pwm.setPWM(7, 0, i);
    }
    delay(500);
    for (uint16_t i=4096; i>=1024; i--){
      pwm.setPWM(7, 0, i);
    }
    delay(500);
  }
  detachInterrupt(INT_PIN);
  turnLEDon();
  delay(100);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);    // set to sleep
}

// INT0 interrupt callback, gets called to wake up from deep sleep
void wakeyWakey(){}

void turnLEDon(){
  pwm.setPWM(7, 4096, 0); // LED on
}

void turnLEDoff(){
  pwm.setPWM(7, 0, 4096); // LED off
}

void displayDaysLeft(uint16_t num){
  if(num < 0){
    // should never end up in here, just in case..
    Serial.println("why are u trying to print negative numbers??");
    terminateProgram();
  }
  if(num > 99){
    num = 99;
    turnLEDon();
  }else{
    turnLEDoff();
  }

  displayNumber(num/10, tensNeutralState, tensTurnedState, false);  // display tens
  displayNumber(num%10, onesNeutralState, onesTurnedState, true);   // display ones
}

void displayNumber(uint8_t num, const uint16_t neutralState[7], const uint16_t turnedState[7], bool ones){
  uint8_t firstPWM; // channel number of first segment in number
  if(ones) firstPWM = 8;
  else firstPWM = 0;

  // move lower side segemnts out of the way, switch middle segment
  if(((num==0 or num==1 or num==7) and (pwm.getPWM(firstPWM+6) != neutralState[6])) or
    ((num==2 or num==3 or num==4 or num==5 or num==6 or num==8 or num==9) and (pwm.getPWM(firstPWM+6) != turnedState[6]))){
    pwm.setPWM(firstPWM+3, 0, neutralState[3]);
    pwm.setPWM(firstPWM+5, 0, neutralState[5]);
    if(num==0 or num==1 or num==7) pwm.setPWM(firstPWM+6, 0, neutralState[6]);
    else pwm.setPWM(firstPWM+6, 0, turnedState[6]);
  }

  // define state of segments in to be displayed number
  bool turnSegments[6]; // {top right, top, top left, bottom left, bottom, bottom right}
  switch (num){
    case 1:
      bool turnSegments[6] = {1, 0, 0, 0, 0, 1};
      break;
    case 2:
      bool turnSegments[6] = {1, 1, 0, 1, 1, 0};
      break;
    case 3:
      bool turnSegments[6] = {1, 1, 0, 0, 1, 1};
      break;
    case 4:
      bool turnSegments[6] = {1, 0, 1, 0, 0, 1};
      break;
    case 5:
      bool turnSegments[6] = {0, 1, 1, 0, 1, 1};
      break;
    case 6:
      bool turnSegments[6] = {0, 1, 1, 1, 1, 1};
      break;
    case 7:
      bool turnSegments[6] = {1, 1, 0, 0, 0, 1};
      break;
    case 8:
      bool turnSegments[6] = {1, 1, 1, 1, 1, 1};
      break;
    case 9:
      bool turnSegments[6] = {1, 1, 1, 0, 1, 1};
      break;
    case 0:
      bool turnSegments[6] = {1, 1, 1, 1, 1, 1};
      break;
    default:
      bool turnSegments[6] = {0, 0, 0, 0, 0, 0};  // should not end up here but if it does displays nonsense
      break;
  }

  // switch remaining segments
  for(uint8_t i=0; i<6; i++){
    uint16_t updatedPWM;
    uint16_t oldState = pwm.getPWM(firstPWM + i);
    if(turnSegments[i]) updatedPWM = turnedState[i];
    else updatedPWM = neutralState[i];
    pwm.setPWM(firstPWM + i, 0, updatedPWM);
    if(updatedPWM != oldState) delay(100);
  }
}

void setup() {
  Serial.begin(57600);

  // setup servo driver board
  pwm.begin();
  pwm.setOscillatorFrequency(25000000);
  pwm.setPWMFreq(50);  // Analog servos run at ~50 Hz updates
  turnLEDon();

  // setup interrupt
  pinMode(INT_PIN, INPUT_PULLUP);

  // setup real time clock (rtc), copied from example sketch "pcf8523Countdown"
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  rtc.deconfigureAllTimers();   // Timer configuration is not cleared on an RTC reset due to battery backup!
  
  // get first reading
  DateTime now = rtc.now();
  
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
  uint8_t day = 0;
  uint8_t month = 0;
  for(uint8_t i = 0; i <= 4; i++){
    day += (2^i) * !digitalRead(DAY_PINS[i]);                 // day is 5 bit long
    if(i < 4) month += (2^i) * !digitalRead(MONTH_PINS[i]);   // month is 4 bit long
  }
  uint8_t hour = 1 * !digitalRead(HOUR_PIN_1) + 2 * !digitalRead(HOUR_PIN_2);
  uint8_t year = !digitalRead(YEAR_PIN);

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
  turnLEDoff();
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
    displayDaysLeft(daysToGo);               // display days left
    if(daysToGo == 0){                       // terminate program when goal is reached
      Serial.println("Target date reached, please reset with new date :)");
      terminateProgram();
    }
  }

  // set timer
  if(timeLeft.hours() > 0) rtc.enableCountdownTimer(PCF8523_FrequencyHour, timeLeft.hours());  // >1h remaining
  else rtc.enableCountdownTimer(PCF8523_FrequencyMinute, timeLeft.minutes() + 1);  // <1h remaining, +1 makes sure final hour is definately passed
  Serial.println("Going to sleep. Sleep tight :)");

  attachInterrupt(INT_PIN, wakeyWakey, LOW);                           // set interrupt
  delay(100);                                                          // wait a bit for processes to finish before sleeping
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                 // go to sleep
  detachInterrupt(INT_PIN);                                            // got woken up by alarm
}
