// Date and time functions using a PCF8563 RTC connected via I2C and Wire lib
#include <RTClib.h>                     // adafruits rtc lib
#include <Adafruit_PWMServoDriver.h>    // adafruits servo driver lib
#include <LowPower.h>                   // Rocket Scream Electronics lib for sleeping 
#include <Wire.h>

// global objects and variables
#define INT_PIN 2
#define SERVO_MIN  150    // This is the 'minimum' pulse length count (out of 4096)
#define SERVO_MAX  600    // This is the 'maximum' pulse length count (out of 4096)
uint16_t daysToGo = 800;  // >2 years, ensures "new day" is triggered on first entry in loop()
volatile bool interruptFlag = false;
DateTime targetDate;
RTC_PCF8523 rtc;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();    // servo driver on I2C address 0x40

void terminateProgram(){
  // TODO pulsate led for 10min
  detachInterrupt(INT_PIN);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);    // set to sleep
}

// INT0 interrupt callback; update TODO flag
void wakeyWakey(){
  // interruptFlag = true;
}

void displayNumber(uint16_t num){
  if(num < 0){
    // should not end up in here like ever
    Serial.println("why are u trying to print negative numbers??");
    terminateProgram();
  }
  if(num > 99){
    num = 99;
    // TODO turn LED on
  }else{
    // TODO turn LED off
  }

  // temporary example code from "servo" example-script
  uint8_t servonum = 0; // 0-15
  for (uint16_t pulselen = SERVO_MIN; pulselen < SERVO_MAX; pulselen++) {
    pwm.setPWM(servonum, 0, pulselen);
  }

  delay(500);
  for (uint16_t pulselen = SERVO_MAX; pulselen > SERVO_MIN; pulselen--) {
    pwm.setPWM(servonum, 0, pulselen);
  }
}

void setup() {
  // TODO turn on LED ASAP

  Serial.begin(57600);

  // setup servo driver board
  pwm.begin();
  pwm.setOscillatorFrequency(25000000);
  pwm.setPWMFreq(50);  // Analog servos run at ~50 Hz updates

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
  // TODO turn LED off
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
  if(timeLeft.days() < daysToGo){   //timeLeft was just calculated, daysToGo is used to compare days!
    if(timeLeft.days() < 0) daysToGo = 0;    // catch case when target date has been passed
    else daysToGo = timeLeft.days();         // update date
    displayNumber(daysToGo);                 // display days left
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
