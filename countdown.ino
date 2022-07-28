// Date and time functions using a PCF8563 RTC connected via I2C and Wire lib
#include <RTClib.h>

// global objects and variables
volatile bool interruptFlag = false;
uint8_t days_to_go;
DateTime target_date;
RTC_PCF8523 rtc;


void setup() {
  Serial.begin(9600);

  // setup interrupt
  const uint8_t INT_PIN = 2;
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), wakey_wakey, FALLING);

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
  // TODO leap year
  // round to max length of month
  if(month == 2 && day > 28){
    day = 28;
    Serial.println("Rounded day to 28 because month is set to February!");
  }else if(month % 2 == 1 && day > 30){
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

  // set current days to go
  TimeSpan time_left = target_date - now;
  days_to_go = time_left.days();
}

// INT0 interrupt callback; update TODO flag
void wakey_wakey(void) {
}

void loop() {
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
      Serial.println("Target date reached, will continue blinking LED until reset :)");
    }
  }
  
  delay(3000);
}
