#include <Wire.h>
#include <DS1307.h>

//LED pin setup
int leds[13] = {A3,4,7,8,9,10,11,12,13,A0,A1,A2};
int colors[4] = {6,5,3};

//Pushbutton
int BTN_PIN = A7;
bool BTN_DOWN_SHORT = false;
bool BTN_DOWN_LONG = false;
bool BTN_IGNORE = false;
bool BTN_WAS_DOWN;
unsigned long BTN_DOWN_STARTED = 0;

//Current LED pin output (memory)
int pin_state[3][13] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
};

//Requested LED pin output
int led_state[3][13] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
};

//Needed to keep track of milliseconds
int prev_sec_five = 0;
unsigned long millisec_five = 0;

//Needed to keep track of time edit inputs
int EDIT_TIME_HOUR = 1;
int EDIT_TIME_MIN = 1;

//RTC time
int rtc[7];

//The state to keep track of navigation
String STATE = "";

void setup()
{
  Serial.begin(9600);
  
  STATE = "rtc_setup";
  setupRtc();
  
  STATE = "setup_pins";
  setupPins();
  
  STATE = "led_test";
  ledTest();
}

void loop()
{
    doLoopChores();

    if(STATE == "edit_minute")
    {
        if(BTN_DOWN_LONG)
        {
            //User just finished editing the time, set the entered time in RTC
            setTime(17, 1, 1, EDIT_TIME_HOUR, EDIT_TIME_MIN, 0, 7);
            STATE = "show_time";
        }
        else
        {
            //The time is currently being edited, the user is entering the minute
            EDIT_TIME_MIN = editMinute(EDIT_TIME_MIN);
        }
    }
    else if(STATE == "edit_hour")
    {
        if(BTN_DOWN_LONG)
        {
            //Enter minute edit process
            editMinute(rtc[1]);
            STATE = "edit_minute";
        }
        else
        {
            //The time is currently being edited, the user is entering the hour
            EDIT_TIME_HOUR = editHour(EDIT_TIME_HOUR);
        }
    }
    else
    {
        if(BTN_DOWN_LONG)
        {
            //Enter time editing mode, starting with editing the hour
            editHour(rtc[2]);
            STATE = "edit_hour";
        }
        else
        {
            //Show the current time for 10 milliseconds
            showTime(rtc[2], rtc[1], rtc[0], 10);
        }
    }
}

void setTime(int year, int month, int date, int hour, int minute, int second, int dow)
{
  RTC.stop();
  RTC.set(DS1307_SEC, second);
  RTC.set(DS1307_MIN, minute);
  RTC.set(DS1307_HR, hour);
  RTC.set(DS1307_DATE, date);
  RTC.set(DS1307_MTH, month);
  RTC.set(DS1307_YR, year);
  RTC.set(DS1307_DOW, dow);
  RTC.start();
  RTC.SetOutput(DS1307_SQW32KHZ);
  RTC.get(rtc, true);
}

int editMinute(int value)
{
  int minute_ratio = map(((value % 5)*10), 0, 50, 0, 255);
  int minute = (value/5)-1;
  led_state[2][minute] = 255 - minute_ratio;
  led_state[2][(minute == 11 ? 0 : minute+1)] = minute_ratio;
  showLeds(10);
  
  if(BTN_DOWN_SHORT)
  {
    value = (value > 60 ? value = 1 : value + 1);
  }
  
  return value;
}

int editHour(int value)
{
  led_state[0][value-1] = 255;
  showLeds(10);

  if(BTN_DOWN_SHORT)
  {
    value = (value > 11 ? value = 1 : value + 1);
  }
  
  return value;
}

void setupPins()
{
  for (int pin=0; pin < 12; pin++)
  {
    pinMode(leds[pin], OUTPUT);
    digitalWrite(leds[pin], HIGH);
  }

  for (int color=0; color < 3; color++)
  {
      pinMode(colors[color], OUTPUT);
      digitalWrite(colors[color], LOW);
  }
}

void showLeds(unsigned long milliseconds)
{
  unsigned long starttime = millis();
  unsigned long endtime = starttime;
  while((endtime - starttime) <= milliseconds)
  {
    for(int p=0; p < 12; p++)
    {
      for(int c=0; c < 3 ; c++)
      {
        if(led_state[c][p] > 0)
        {
          setLed(p, c, 255);
          delayMicroseconds(map(led_state[c][p], 0, 255, 1, 2000));
        }
        setLed(p, c, 0);
      }
    }
    endtime = millis();
  }
  
  for(int p=0; p < 12; p++)
  {
    for(int c=0; c < 3 ; c++)
    {
      led_state[c][p] = 0;
    }
  }
}

void setLed(int hour, int col, int intensity)
{
  if(hour < 0 || hour > 11)
  {
    return;
  }

  if(pin_state[col][hour] != intensity)
  {
    analogWrite(colors[col], intensity);
    
    if(intensity == 0)
    {
      digitalWrite(leds[hour], HIGH);
      
    }
    else
    {
      digitalWrite(leds[hour], LOW);
    }
    pin_state[col][hour] = intensity;
  }
}

void ledTest()
{
  for(int c=0; c < 3 ; c++)
  {
    for(int p=0; p < 12; p++)
    {
      led_state[c][p] = 255;
      showLeds(50);
    }
  }
}

void setupRtc()
{
  DDRC |= _BV(2) | _BV(3); // POWER:Vcc Gnd
  PORTC |= _BV(3); // VCC PINC3
  
  RTC.get(rtc, true);
  if(rtc[6] < 12)
  {
    RTC.stop();
    RTC.set(DS1307_SEC, 1);
    RTC.set(DS1307_MIN, 27);
    RTC.set(DS1307_HR, 01);
    RTC.set(DS1307_DOW, 7);
    RTC.set(DS1307_DATE, 12);
    RTC.set(DS1307_MTH, 2);
    RTC.set(DS1307_YR, 12);
    RTC.start();
  }
  RTC.SetOutput(DS1307_SQW32KHZ);
  RTC.get(rtc, true);
}

void doLoopChores()
{
    //Is the button being press right now
    bool btn_is_pressed = analogRead(BTN_PIN) > 100 ? false : true;

    //If a long press was registered in the last loop, and user is still pushing, ignore it
    if(!BTN_IGNORE)
    {
        //If the button is being pressed now and was not being pressed before, start counting press time
        if(btn_is_pressed && !BTN_WAS_DOWN)
        {
            BTN_DOWN_STARTED = millis();
        }

        //If button is not pressed now and was pressed before, register short press
        if(!btn_is_pressed && BTN_WAS_DOWN)
        {
            BTN_DOWN_SHORT = true;
        }
        else
        {
            BTN_DOWN_SHORT = false;
        }
      
        //If button is pressed now and has been pressed for more than 2 sec, register long press
        if(btn_is_pressed && BTN_WAS_DOWN && (millis() - BTN_DOWN_STARTED) > 2000)
        {
            BTN_DOWN_LONG = true;
            
            //Ignore the button press until the user lets go
            BTN_IGNORE = true;
        }
        else
        {
            BTN_DOWN_LONG = false;
        }
    }
    
    //If the user last did a long press and now they have let go, do not ignore button press anymore
    if(!btn_is_pressed)
    {
        BTN_IGNORE = false;
    }
    
    //Refresh RTC time buffer every second
    if(millis() - millisec_five == 0 || millis() - millisec_five > 1000)
    {
        RTC.get(rtc, true);
    }
    else
    {
        RTC.get(rtc, false);
    }

  //Update milliseconds in multiples of 5 seconds
  if(rtc[0]/5 != prev_sec_five)
  {
    prev_sec_five = rtc[0]/5;
    millisec_five = millis();
  }
}

void showTime(int show_hour, int show_minute, int show_second, unsigned long milliseconds)
{
  int second_ratio = map(millis() - millisec_five, 0, 5000, 0, 255);
  led_state[1][show_second/5] = 255 - second_ratio;
  led_state[1][((show_second/5) == 11 ? 0 : (show_second/5)+1)] = second_ratio;

  int minute_ratio = map(((show_minute % 5)*10), 0, 50, 0, 255);
  int minute = (show_minute/5)-1;
  led_state[2][minute] = 255 - minute_ratio;
  led_state[2][(minute == 11 ? 0 : minute+1)] = minute_ratio;

  int hour_ratio = map(show_minute, 0, 60, 0, 255);
  int hour = (show_hour > 12 ? show_hour-12 : show_hour)-1;
  led_state[0][hour] = 255 - hour_ratio;
  led_state[0][((hour) == 12 ? 0 : hour+1)] = hour_ratio;
  
  showLeds(milliseconds);
}

