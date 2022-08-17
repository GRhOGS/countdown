// Date and time functions using a PCF8563 RTC connected via I2C and Wire lib
#include <RTClib.h>
#include <LowPower.h>

// global objects and variables
volatile bool interrupt_flag = false;
volatile bool first_interrupt = false;
uint8_t days_to_go;
DateTime target_date;
RTC_PCF8523 rtc;


void setup() {
  Serial.begin(57600);

  // setup interrupt
  const uint8_t INT_PIN = 2;
  pinMode(INT_PIN, INPUT_PULLUP);

  // setup real time clock (rtc), copied from example sketch "pcf8523Countdown"
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  rtc.deconfigureAllTimers();   // Timer configuration is not cleared on an RTC reset due to battery backup!
  
  // get first reading
  DateTime now = rtc.now();
  
  // setup input pins for date selection
  // month
  const uint8_t MONTH_PINS[4] = {3, 4, 5, 6};
  // day
  const uint8_t DAY_PINS[5] = {8, 9, 10, 11, 12};
  // hour, year
  const uint8_t HOUR_PIN_1 = A1;
  const uint8_t HOUR_PIN_2 = A0;
  const uint8_t YEAR_PIN = A3;

  //setup pinmodes
  for(int i=0; i<=3; i++){
    pinMode(MONTH_PINS[i], INPUT_PULLUP);
  }
  for(int i=0; i<=4; i++){
    pinMode(DAY_PINS[i], INPUT_PULLUP);
  }
  pinMode(HOUR_PIN_1, INPUT_PULLUP);
  pinMode(HOUR_PIN_2, INPUT_PULLUP);
  pinMode(YEAR_PIN, INPUT_PULLUP);

  // convvert selected date from binary to int, save in variables
  int day = 1 * !digitalRead(DAY_PINS[0]) + 2 * !digitalRead(DAY_PINS[1]) + 4 * !digitalRead(DAY_PINS[2]) + 8 * !digitalRead(DAY_PINS[3]) + 16 * !digitalRead(DAY_PINS[4]);
  int month = 1 * !digitalRead(MONTH_PINS[0]) + 2 * !digitalRead(MONTH_PINS[1]) + 4 * !digitalRead(MONTH_PINS[2]) + 8 * !digitalRead(MONTH_PINS[3]);
  int hour = 1 * !digitalRead(HOUR_PIN_1) + 2 * !digitalRead(HOUR_PIN_2);
  int year = !digitalRead(YEAR_PIN);

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
  target_date = DateTime(year, month, day, hour, 0, 0); // year, month, day, hour, minute, second

  // print target date
  Serial.print("target date: ");
  Serial.print(target_date.day());
  Serial.print(".");
  Serial.print(target_date.month());
  Serial.print(".");
  Serial.print(target_date.year());
  Serial.print(", hour: ");
  Serial.println(target_date.hour());

  // save current days to go for comparison in main loop
  TimeSpan time_left = target_date - now;
  days_to_go = time_left.days();

  // trigger first rtc interrupt, so that subsequent interrupts are accurate
  rtc.enableCountdownTimer(PCF8523_FrequencySecond, 2);              // set 2 second timer
  attachInterrupt(digitalPinToInterrupt(2), first_wake, FALLING);     // interrupt function  "first_wake"
  while(!first_interrupt){
    Serial.println(millis());
    delay(200);
  }
  Serial.print("First Interrupt successfully triggered at ");
  Serial.println(millis());
  detachInterrupt(digitalPinToInterrupt(2));
}

//------ interrupt procedures ------
// INT0 interrupt callback; tell programm to continue setup phase
void first_wake(){
  first_interrupt = true;
}

// INT0 interrupt callback; update TODO flag
void wakey_wakey(){
  interrupt_flag = true;
}

void loop() {
  rtc.deconfigureAllTimers();     // turn off alarm
  DateTime now = rtc.now();
  TimeSpan time_left = target_date - now;
  Serial.print("time left: ");
  Serial.print(time_left.days());
  Serial.print(" days ");
  Serial.print(time_left.hours());
  Serial.print(" hours ");
  Serial.print(time_left.minutes());
  Serial.println(" minutes.");

  // on new day
  if((time_left.days() < days_to_go) or (time_left.days() <= 0)){
    days_to_go = time_left.days();              //update date
    if(time_left.days() < 0) days_to_go = 0;    // catch case when target date has been passed

    
    // TODO switch number

    // blink led forever when target date is reached
    if(days_to_go == 0){
      Serial.println("Target date reached, will continue blinking LED until reset with new date :)");
    }
  }

  // sleep until next hour
  uint16_t minutes_to_go = 59 - now.minute();                          // calculate minutes to wait
  Serial.print("Waking up in ");
  Serial.print(minutes_to_go);
  Serial.println(" minutes. Sleep tight :)");
  rtc.enableCountdownTimer(PCF8523_FrequencyMinute, minutes_to_go);    // set timer
  attachInterrupt(digitalPinToInterrupt(2), wakey_wakey, LOW);         // interrupt function  "wakey_wakey"
  delay(500);                                                          // wait a bit for processes to finish before sleeping
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                 // set to sleep
  detachInterrupt(digitalPinToInterrupt(2));                           // woke up

}
