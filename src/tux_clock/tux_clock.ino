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
bool btn_was_down = false;
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

//Keep track of show date
int show_date_speed = 4000; //in milliseconds
unsigned long show_date_last_millis = 0;

//Needed to keep track of datetime editing
unsigned long edit_date_blink_millis = 0;
int edit_date_year = 0;
int edit_date_month = 0;
int edit_date_day = 0;
int edit_time_hour = 0;
int edit_time_min = 0;
int edit_weekday = 0;

//RTC time
int rtc[7]; //sec,min,hour,dow,day,month,year
unsigned long rtc_last_refreshed = 0;

//The state to keep track of navigation
String state = "";

void setup()
{
    Serial.begin(9600);

    state = "rtc_setup";
    setupRtc();

    //setDateTime(rtc[0], rtc[1], rtc[2], rtc[3], rtc[4], rtc[5], rtc[6]);
    
    state = "setup_pins";
    setupPins();

    state = "led_test";
    ledTest();
    
    state = "show_time";
}

void loop()
{
    checkButtonState();
    Serial.print(edit_time_hour);
    Serial.print(" ");
    Serial.print(edit_time_min);
    Serial.print(" ");
    Serial.println(state);
    
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
    
    if(state == "edit_time")
    {
        state = "edit_hour";
    }
    else if(state == "edit_date")
    {
        state = "edit_month";
    }
    else if(state == "show_date")
    {
        show_date_last_millis = millis();
        state = "show_month";
    }
    else if(state == "edit_hour")
    {
        if(btn_down_long)
        {
            state = "edit_minute";
        }
        else
        {
            editHour();
        }
    }
    else if(state == "edit_minute")
    {
        if(btn_down_long)
        {
            //User just finished editing the time, set the entered time in RTC
            setDateTime(0, edit_time_min, edit_time_hour, rtc[3], rtc[4], rtc[5], rtc[6]);
            state = "show_time";
        }
        else
        {
            editMinute();
        }
    }
    else if(state == "show_month")
    {
        if(btn_down_short)
        {
            state = "show_time";
        }
        else if(btn_down_long)
        {
            state = "edit_date";
        }
        else if(millis() - show_date_last_millis > show_date_speed)
        {
            show_date_last_millis = millis();
            state = "show_day_1";
        }
        else
        {
            showMonth(rtc[5]);
        }
    }
    else if(state == "show_day_1")
    {
        if(btn_down_short)
        {
            state = "show_time";
        }
        else if(btn_down_long)
        {
            state = "edit_date";
        }
        else if(millis() - show_date_last_millis > show_date_speed)
        {
            show_date_last_millis = millis();
            state = "show_day_2";
        }
        else
        {
            showDay(rtc[4], 1);
        }
    }
    else if(state == "show_day_2")
    {
        if(btn_down_short)
        {
            state = "show_time";
        }
        else if(btn_down_long)
        {
            state = "edit_date";
        }
        else if(millis() - show_date_last_millis > show_date_speed)
        {
            show_date_last_millis = millis();
            state = "show_year_1";
        }
        else
        {
            showDay(rtc[4], 2);
        }
    }
    else if(state == "show_year_1")
    {
        if(btn_down_short)
        {
            state = "show_time";
        }
        else if(btn_down_long)
        {
            state = "edit_date";
        }
        else if(millis() - show_date_last_millis > show_date_speed)
        {
            show_date_last_millis = millis();
            state = "show_year_2";
        }
        else
        {
            showYear(rtc[6], 1);
        }
    }
    else if(state == "show_year_2")
    {
        if(btn_down_short)
        {
            state = "show_time";
        }
        else if(btn_down_long)
        {
            state = "edit_date";
        }
        else if(millis() - show_date_last_millis > show_date_speed)
        {
            show_date_last_millis = millis();
            state = "show_month";
        }
        else
        {
            showYear(rtc[6], 2);
        }
    }
    else if(state == "edit_month")
    {
        if(btn_down_long)
        {
            state = "edit_day_1";
        }
        else
        {
            editMonth();
        }
    }
    else if(state == "edit_day_1")
    {
        if(btn_down_long)
        {
            state = "edit_day_2";
        }
        else
        {
            editDay(1);
        }
    }
    else if(state == "edit_day_2")
    {
        if(btn_down_long)
        {
            state = "edit_year_1";
        }
        else
        {
            editDay(2);
        }
    }
    else if(state == "edit_year_1")
    {
        if(btn_down_long)
        {
            state = "edit_year_2";
        }
        else
        {
            editYear(1);
        }
    }
    else if(state == "edit_year_2")
    {
        if(btn_down_long)
        {
            setDateTime(0, rtc[1], rtc[2], rtc[3], edit_date_day, edit_date_month, edit_date_year);
            state = "show_date";
        }
        else
        {
            editYear(2);
        }
    }
    else
    {
        if(btn_down_short)
        {
            state = "show_date";
        }
        else if(btn_down_long)
        {
            state = "edit_time";
        }
        
        showTime(rtc[2], rtc[1], rtc[0]);
    }
}

void setDateTime(int sec, int min, int hour, int dow, int day, int month, int year)
{
    RTC.stop();
    RTC.set(DS1307_SEC, sec);
    RTC.set(DS1307_MIN, min);
    RTC.set(DS1307_HR, hour);
    RTC.set(DS1307_DOW, dow);
    RTC.set(DS1307_DATE, day);
    RTC.set(DS1307_MTH, month);
    RTC.set(DS1307_YR, year);
    
    RTC.start();
    RTC.SetOutput(DS1307_SQW32KHZ);
    RTC.get(rtc, true);
    
    edit_date_year = 0;
    edit_date_month = 0;
    edit_date_day = 0;
    edit_time_hour = 0;
    edit_time_min = 0;
    edit_weekday = 0;
}

void editMonth()
{
    if(edit_date_month == 0)
    {
        edit_date_month = rtc[5];
    }
    else if(btn_down_short)
    {
        edit_date_month = (edit_date_month >= 12 ? edit_date_month = 1 : edit_date_month + 1);
    }
    
    if(millis() - edit_date_blink_millis > 2000)
    {
        edit_date_blink_millis = millis();
    }
    int intensity = (millis() - edit_date_blink_millis < 1000 ? 255 : 50);
    
    ledSet('r', edit_date_month-8, intensity);
    ledSet('r', edit_date_month-7, intensity);
    ledSet('r', edit_date_month-6, 10);
    ledSet('g', edit_date_month-6, 10);
    ledSet('r', edit_date_month-5, 50);
    ledSet('r', edit_date_month-4, 50);
    ledSet('r', edit_date_month-3, 10);
    ledSet('g', edit_date_month-3, 10);
    ledSet('r', edit_date_month-2, 50);
    ledSet('r', edit_date_month-1, 50);
    ledSet('b', edit_date_month, 255);
    showLeds(40);
}

void editDay(int position)
{
    if(btn_down_short)
    {
        if(position == 1)
        {
            edit_date_day = (edit_date_day >= 22 ? edit_date_day = 0 : edit_date_day + 10);
        }
        else
        {
            edit_date_day = (nthClockDigit(edit_date_day, 2) >= 9 ? edit_date_day = edit_date_day-9 : edit_date_day + 1);
        }
    }
    
    if(millis() - edit_date_blink_millis > 2000)
    {
        edit_date_blink_millis = millis();
    }
    int intensity = (millis() - edit_date_blink_millis < 1000 ? 255 : 50);
    
    int show_day = nthClockDigit(edit_date_day, position);
    int start_ind = (show_day != 0) ? show_day : 1;
    
    ledSet('r', start_ind-8, 50);
    ledSet('r', start_ind-7, 50);
    ledSet('r', start_ind-6, 10);
    ledSet('g', start_ind-6, 10);
    ledSet('r', start_ind-5, (position == 1 ? intensity : 50));
    ledSet('r', start_ind-4, (position == 2 ? intensity : 50));
    ledSet('r', start_ind-3, 10);
    ledSet('g', start_ind-3, 10);
    ledSet('r', start_ind-2, 50);
    ledSet('r', start_ind-1, 50);
    if(show_day != 0)
    {
        ledSet('b', show_day, 255);
    }
    showLeds(40);
}

void editYear(int position)
{
    if(edit_date_year > 99){ edit_date_year = edit_date_year % 100; }
    
    if(btn_down_short)
    {
        if(position == 1)
        {
            edit_date_year = (edit_date_year >= 90 ? edit_date_year = 0 : edit_date_year + 10);
        }
        else
        {
            edit_date_year = (nthClockDigit(edit_date_year, 2) >= 9 ? edit_date_year = edit_date_year-9 : edit_date_year + 1);
        }
    }
    
    if(millis() - edit_date_blink_millis > 2000)
    {
        edit_date_blink_millis = millis();
    }
    int intensity = (millis() - edit_date_blink_millis < 1000 ? 255 : 50);
    
    int show_year = nthClockDigit(edit_date_year, position);
    int start_ind = (show_year != 0) ? show_year : 1;
    
    ledSet('r', start_ind-8, 50);
    ledSet('r', start_ind-7, 50);
    ledSet('r', start_ind-6, 10);
    ledSet('g', start_ind-6, 10);
    ledSet('r', start_ind-5, 50);
    ledSet('r', start_ind-4, 50);
    ledSet('r', start_ind-3, 10);
    ledSet('g', start_ind-3, 10);
    ledSet('r', start_ind-2, (position == 1 ? intensity : 50));
    ledSet('r', start_ind-1, (position == 2 ? intensity : 50));
    if(show_year != 0)
    {
        ledSet('b', show_year, 255);
    }
    showLeds(40);
}

void editHour()
{
    if(edit_time_hour == 0)
    {
        edit_time_hour = rtc[2];
        edit_time_hour = (edit_time_hour > 12 ? edit_time_hour-12 : edit_time_hour);
    }
    
    if(btn_down_short)
    {
        edit_time_hour = (edit_time_hour > 11 ? edit_time_hour = 1 : edit_time_hour + 1);
    }
    
    ledSet('r', edit_time_hour, 255);
    showLeds(40);
}

void editMinute()
{
    if(edit_time_min == 0)
    {
        edit_time_min = rtc[1];
    }
    
    if(btn_down_short)
    {
        edit_time_min = (edit_time_min > 60 ? edit_time_min = 1 : edit_time_min + 1);
    }
    
    int minute_ratio = map(((edit_time_min % 5)*10), 0, 50, 0, 255);
    int minute = (edit_time_min/5);
    ledSet('b', minute, 255-minute_ratio);
    ledSet('b', minute+1, minute_ratio);
    showLeds(40);
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
    char colors[3] = {'r','g','b'};
    unsigned long starttime = millis();
    unsigned long endtime = starttime;
    while((endtime - starttime) <= milliseconds)
    {
        for(int p=0; p < 12; p++)
        {
            for(int c=0; c < 3 ; c++)
            {
                int intensity_delay = map(ledGet(colors[c], p+1), 0, 255, 0, 2000);
                
                if(intensity_delay > 0)
                {
                    if(pin_state[c][p] == 0)
                    {
                        digitalWrite(LED_ANOD_PIN[c], HIGH);
                        digitalWrite(LED_CATD_PIN[p], LOW);
                        pin_state[c][p] = 1;
                    }
                    
                    delayMicroseconds(intensity_delay);
                    
                    digitalWrite(LED_ANOD_PIN[c], LOW);
                    digitalWrite(LED_CATD_PIN[p], HIGH);
                    pin_state[c][p] = 0;
                }
                if(2000 - intensity_delay > 0)
                {
                    delayMicroseconds((2000 - intensity_delay)/16);
                }
                if(pin_state[c][p] == 1)
                {
                    digitalWrite(LED_ANOD_PIN[c], LOW);
                    digitalWrite(LED_CATD_PIN[p], HIGH);
                    pin_state[c][p] = 0;
                }
            }
        }
        endtime = millis();
    }
  
    for(int p=0; p < 12; p++)
    {
        for(int c=0; c < 3 ; c++)
        {
            ledSet(colors[c], p+1, 0);
        }
    }
}

void ledTest()
{
    char colors[3] = {'r','g','b'};
    for(int c=0; c < 3 ; c++)
    {
        for(int p=0; p < 12; p++)
        {
            ledSet(colors[c], p+1, 255);
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
    int second = (show_second/5);
    ledSet('g', second, 255-second_ratio);
    ledSet('g', second+1, second_ratio);
    
    int minute_ratio = map(((show_minute % 5)*10), 0, 50, 0, 255);
    int minute = (show_minute/5);
    ledSet('b', minute, 255-minute_ratio);
    ledSet('b', minute+1, minute_ratio);
    
    int hour_ratio = map(show_minute, 0, 60, 0, 255);
    int hour = (show_hour > 12 ? show_hour-12 : show_hour);
    ledSet('r', hour, 255-hour_ratio);
    ledSet('r', hour+1, hour_ratio);
    
    showLeds(40);
}

void showMonth(int show_month)
{
    ledSet('r', show_month-8, 255);
    ledSet('r', show_month-7, 255);
    ledSet('r', show_month-6, 10);
    ledSet('g', show_month-6, 10);
    ledSet('r', show_month-5, 30);
    ledSet('r', show_month-4, 30);
    ledSet('r', show_month-3, 10);
    ledSet('g', show_month-3, 10);
    ledSet('r', show_month-2, 30);
    ledSet('r', show_month-1, 30);
    ledSet('b', show_month, 255);
    showLeds(40);
}

void showDay(int show_day, int position)
{
    if(state == "show_day_1" && show_day < 12)
    {
        state = "show_day_2";
    }
    else if(show_day <= 12)
    {
        ledSet('r', show_day-8, 30);
        ledSet('r', show_day-7, 30);
        ledSet('r', show_day-6, 10);
        ledSet('g', show_day-6, 10);
        ledSet('r', show_day-5, 255);
        ledSet('r', show_day-4, 255);
        ledSet('r', show_day-3, 10);
        ledSet('g', show_day-3, 10);
        ledSet('r', show_day-2, 30);
        ledSet('r', show_day-1, 30);
        ledSet('b', show_day, 255);
        showLeds(40);
    }
    else
    {
        show_day = nthClockDigit(show_day, position);
        int start_ind = (show_day != 0) ? show_day : 1;
        
        ledSet('r', start_ind-8, 30);
        ledSet('r', start_ind-7, 30);
        ledSet('r', start_ind-6, 10);
        ledSet('g', start_ind-6, 10);
        ledSet('r', start_ind-5, (position == 1 ? 255 : 30));
        ledSet('r', start_ind-4, (position == 2 ? 255 : 30));
        ledSet('r', start_ind-3, 10);
        ledSet('g', start_ind-3, 10);
        ledSet('r', start_ind-2, 30);
        ledSet('r', start_ind-1, 30);
        if(show_day != 0)
        {
            ledSet('b', show_day, 255);
        }
        showLeds(40);
    }
}

void showYear(int show_year, int position)
{
    if(show_year > 99){ show_year = show_year % 100; }
    
    if(state == "show_year_1" && show_year < 12)
    {
        state = "show_year_2";
    }
    else if(show_year <= 12)
    {
        ledSet('r', show_year-8, 30);
        ledSet('r', show_year-7, 30);
        ledSet('r', show_year-6, 10);
        ledSet('g', show_year-6, 10);
        ledSet('r', show_year-5, 30);
        ledSet('r', show_year-4, 30);
        ledSet('r', show_year-3, 10);
        ledSet('g', show_year-3, 10);
        ledSet('r', show_year-2, 255);
        ledSet('r', show_year-1, 255);
        ledSet('b', show_year, 255);
        showLeds(40);
    }
    else
    {
        show_year = nthClockDigit(show_year, position);
        int start_ind = (show_year != 0) ? show_year : 1;
        
        ledSet('r', start_ind-8, 30);
        ledSet('r', start_ind-7, 30);
        ledSet('r', start_ind-6, 10);
        ledSet('g', start_ind-6, 10);
        ledSet('r', start_ind-5, 30);
        ledSet('r', start_ind-4, 30);
        ledSet('r', start_ind-3, 10);
        ledSet('g', start_ind-3, 10);
        ledSet('r', start_ind-2, (position == 1 ? 255 : 30));
        ledSet('r', start_ind-1, (position == 2 ? 255 : 30));
        if(show_year != 0)
        {
            ledSet('b', show_year, 255);
        }
        showLeds(40);
    }
}

int nthClockDigit(int x, int n)
{
    if(x <= 0 || x >= 100){ return 0; }
    if(n == 2 && x < 10){ return x; } 
    if(n == 2 && x > 9 && x <= 19){ return x - 10; } 
    if(n == 2 && x > 19 && x <= 29){ return x - 20; } 
    if(n == 2 && x > 29 && x <= 39){ return x - 30; } 
    if(n == 2 && x > 39 && x <= 49){ return x - 40; } 
    if(n == 2 && x > 49 && x <= 59){ return x - 50; } 
    if(n == 2 && x > 59 && x <= 69){ return x - 60; } 
    if(n == 2 && x > 69 && x <= 79){ return x - 70; } 
    if(n == 2 && x > 79 && x <= 89){ return x - 80; } 
    if(n == 2 && x > 89 && x <= 99){ return x - 90; } 
    if(n == 1 && x < 10){ return 0; }
    if(n == 1 && x > 9 && x < 20){ return 1; }
    if(n == 1 && x > 19 && x < 30){ return 2; }
    if(n == 1 && x > 29 && x < 40){ return 3; }
    if(n == 1 && x > 39 && x < 50){ return 4; }
    if(n == 1 && x > 49 && x < 60){ return 5; }
    if(n == 1 && x > 59 && x < 70){ return 6; }
    if(n == 1 && x > 69 && x < 80){ return 7; }
    if(n == 1 && x > 79 && x < 90){ return 8; }
    if(n == 1 && x > 89 && x < 100){ return 9; }
}

void ledSet(char color, int led, int intensity)
{
    int c = 0;
    if(led > 12){ led = led - 12; }
    if(led < 1){ led = led + 12; }
    if(color == 'r'){ c = 0; }
    else if(color == 'g'){ c = 1; }
    else if(color == 'b'){ c = 2; }
    led_state[c][led-1] = intensity;
}

int ledGet(char color, int led)
{
    int c = 0;
    if(led > 12){ led = led - 12; }
    if(led < 1){ led = led + 12; }
    if(color == 'r'){ c = 0; }
    else if(color == 'g'){ c = 1; }
    else if(color == 'b'){ c = 2; }
    return led_state[c][led-1];
}

void checkButtonState()
{
    //Is the button being press right now
    bool btn_is_pressed = analogRead(BTN_PIN) > 100 ? false : true;

    //If the button long press was registered in the last loop, stop it in this loop
    if(btn_down_long)
    {
        btn_down_long = false;
    }
    
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
    if(btn_was_down && !btn_is_pressed)
    {
        btn_ignore = false;
    }
    
    //Let this loops button state be known in the next loop
    btn_was_down = btn_is_pressed;
}