#include <Wire.h>

//Library found at:
//http://www.dfrobot.com/image/data/DFR0151/V1.1/Arduino%20library.zip
#include <DS1307.h>

//Pin setup
const int LED_CATD_PIN[13] = {A3,4,7,8,9,10,11,12,13,A0,A1,A2};
const int LED_ANOD_PIN[4] = {6,5,3};
const int BTN_PIN = A7;

//Pushbutton
bool btn_down_short = false;
bool btn_down_long = false;
bool btn_ignore = false;
bool btn_was_down;
unsigned long btn_down_started = 0;

//Contains the current led output
int pin_state[3][13] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
};

//Contains led output to be drawn on the next request
int led_state[3][13] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
};

//Keep track of milliseconds between seconds
int last_sec_five = 0;
unsigned long last_sec_five_millis = 0;

//Needed to keep track of time edit inputs
int edit_time_hour = 1;
int edit_time_min = 1;

//RTC time
int rtc[7];
unsigned long rtc_last_refreshed = 0;

//The state to keep track of navigation
String state = "";

void setup()
{
    Serial.begin(9600);

    state = "rtc_setup";
    setupRtc();

    state = "setup_pins";
    setupPins();

    state = "led_test";
    ledTest();
}

void loop()
{
    checkButtonState();

    //Refresh RTC time buffer every minute
    if(millis() - rtc_last_refreshed > 1000 || rtc_last_refreshed > millis())
    {
        rtc_last_refreshed = millis();
        RTC.get(rtc, true);
    }
    else
    {
        RTC.get(rtc, false);
    }
    
    //Keep track of milliseconds between each multiples of 5 seconds
    if(rtc[0]/5 != last_sec_five)
    {
        last_sec_five = rtc[0]/5;
        last_sec_five_millis = millis();
    }
    
    if(state == "edit_minute")
    {
        if(btn_down_long)
        {
            //User just finished editing the time, set the entered time in RTC
            setTime(17, 1, 1, edit_time_hour, edit_time_min, 0, 7);
            state = "show_time";
        }
        else
        {
            //The time is currently being edited, the user is entering the minute
            edit_time_min = editMinute(edit_time_min);
        }
    }
    else if(state == "edit_hour")
    {
        if(btn_down_long)
        {
            //Enter minute edit process
            editMinute(rtc[1]);
            state = "edit_minute";
        }
        else
        {
            //The time is currently being edited, the user is entering the hour
            edit_time_hour = editHour(edit_time_hour);
        }
    }
    else
    {
        if(btn_down_long)
        {
            //Enter time editing mode, starting with editing the hour
            editHour(rtc[2]);
            state = "edit_hour";
        }
        else
        {
            showTime(rtc[2], rtc[1], rtc[0]);
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

    if(btn_down_short)
    {
        value = (value > 60 ? value = 1 : value + 1);
    }

    return value;
}

int editHour(int value)
{
    led_state[0][value-1] = 255;
    showLeds(10);

    if(btn_down_short)
    {
        value = (value > 11 ? value = 1 : value + 1);
    }

    return value;
}

void setupPins()
{
    for(int pin=0; pin < 12; pin++)
    {
        pinMode(LED_CATD_PIN[pin], OUTPUT);
        digitalWrite(LED_CATD_PIN[pin], HIGH);
    }

    for(int color=0; color < 3; color++)
    {
        pinMode(LED_ANOD_PIN[color], OUTPUT);
        digitalWrite(LED_ANOD_PIN[color], LOW);
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
                    if(p >= 0 || p <= 11)
                    {
                        if(pin_state[c][p] != 255)
                        {
                            digitalWrite(LED_ANOD_PIN[c], HIGH);
                            digitalWrite(LED_CATD_PIN[p], LOW);
                            pin_state[c][p] = 255;
                        }
                    }
                    delayMicroseconds(map(led_state[c][p], 0, 255, 1, 2000));
                }

                if(p >= 0 || p <= 11)
                {
                    if(pin_state[c][p] != 0)
                    {
                        digitalWrite(LED_ANOD_PIN[c], LOW);
                        digitalWrite(LED_CATD_PIN[p], HIGH);
                        pin_state[c][p] = 0;
                    }
                }
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

void showTime(int show_hour, int show_minute, int show_second)
{
    int second_ratio = map(millis() - last_sec_five_millis, 0, 5000, 0, 255);
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
    
    showLeds(40);
}

void checkButtonState()
{
    //Is the button being press right now
    bool btn_is_pressed = analogRead(BTN_PIN) > 100 ? false : true;

    //If a long press was registered in the last loop, and user is still pushing, ignore it
    if(!btn_ignore)
    {
        //If the button is being pressed now and was not being pressed before, start counting press time
        if(btn_is_pressed && !btn_was_down)
        {
            btn_down_started = millis();
        }

        //If button is not pressed now and was pressed before, register short press
        if(!btn_is_pressed && btn_was_down)
        {
            btn_down_short = true;
        }
        else
        {
            btn_down_short = false;
        }
      
        //If button is pressed now and has been pressed for more than 2 sec, register long press
        if(btn_is_pressed && btn_was_down && (millis() - btn_down_started) > 2000)
        {
            btn_down_long = true;
            
            //Ignore the button press until the user lets go
            btn_ignore = true;
        }
        else
        {
            btn_down_long = false;
        }
    }
    
    //If the user last did a long press and now they have let go, do not ignore button press anymore
    if(!btn_is_pressed)
    {
        btn_ignore = false;
    }
}