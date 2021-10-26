/*
 * project: Sunrise w/neoPIX
 * name:Casey
 */

#include <TimeLib.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "TM1637.h"
#include <Adafruit_NeoPixel.h>
#include <colors.h>

const int PIXELPIN = 17;
const int PIXELCOUNT = 1;

#define ON 1
#define OFF 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define BME_ADDRESS 0x76

#define CLK 0 //pins for 4 digit display
#define DIO 1

Adafruit_NeoPixel pixel(PIXELCOUNT, PIXELPIN, NEO_GRB + NEO_KHZ800);
TM1637 tm1637(CLK,DIO);
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT, &Wire);
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

unsigned long currentTime, lastSecond, sunRiseTimer;

int alarmHours = 12; 
int alarmMinutes = 30;

int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
unsigned char ClockPoint = 1;

bool sunRiseOn, prevSunState;

void setup() {
  setSyncProvider(getTeensy3Time);
  tm1637.set();
  tm1637.init();

  Serial.begin(9600);
  while(!Serial);
  if (timeStatus()!= timeSet) {
     Serial.println("Unable to sync with the RTC");
  }
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.printf("SSD1306 allocation failed");
    for(;;);
  }

  display.display();
  display.clearDisplay();
  display.display();

  pixel.begin();
  pixel.show();
}

void loop() {
    currentTime = millis();   
  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
  if ((currentTime - lastSecond)>1000) {
    OLEDClockDisplay();
    timeUpdate();
    lastSecond = millis();
  }
  sunRiseLite();
  sunRiseOn = false;
}

void sunRiseLite (void) {
  unsigned long beginSunRiseMinutes,sunRiseTimer,sunRiseElapsed,sunRiseHour,sunRiseMinutes;
  int earlyLUX,lateLUX;
 
  
  beginSunRiseMinutes = ((alarmMinutes)-10);
  if (beginSunRiseMinutes >= 0) {
    if ((alarmHour == hour()) && (beginSunRiseMinutes == minute()) && (second() == 0)) {
      sunRiseOn = !sunRiseOn;
      delay(1000);
      
      if (sunRiseOn) {
        Serial.printf("sunRiseOn=%i,elapsed time=%i\n",sunRiseOn,sunRiseElapsed);
        sunRiseTimer = millis();
        
        sunRiseElapsed = ((currentTime)-(sunRiseTimer));
          if (sunRiseElapsed < 5000) {
            earlyLUX = map(sunRiseElapsed,0,5000,0,30);
            pixel.setPixelColor(0, blue);
            pixel.setBrightness(earlyLUX);
            pixel.show();
          }
          if (sunRiseElapsed > 5000) {
            lateLUX = map(sunRiseElapsed,5000,10000,30,255);
            pixel.setPixelColor(0, yellow);
            pixel.setBrightness(lateLUX);
            pixel.show()
          }
      }
    }
  }
  if (beginSunRiseMinutes < 0) {
    sunRiseHour = ((alarmHours) - 1);
    sunRiseMinutes = (beginSunRiseMinutes +60);
    if ((sunRiseHour == hour()) && (sunRiseMinutes == minute())) {
      sunRiseTimer = millis();
      sunRiseElapsed = ((currentTime)-(sunRiseTimer));
        if (sunRiseElapsed < 5000) {
          earlyLUX = map(sunRiseElapsed,0,5000,0,75);
          
        }
        if (sunRiseElapsed > 5000) {
          lateLUX = map(sunRiseElapsed,5000,10000,75,255);
     
        }
    }
  }
}

void OLEDClockDisplay(void) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,50);
  display.printf("%i",hour());
  OLEDprintDigits(minute());
  OLEDprintDigits(second());
  display.printf(" ");
  display.printf("%i",day());
  display.printf(" ");
  display.printf("%i",month());
  display.printf(" ");
  display.printf("%i",year()); 
  display.printf(""); 
  display.display();
}

void OLEDprintDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  display.printf(":");
  if(digits < 10)
    display.printf("0");
  display.printf("%i",digits);
}


void timeUpdate(void) { //time display to 4_Digit_Display
  TimeDisp[0] = hour() / 10;
  TimeDisp[1] = hour() % 10;
  TimeDisp[2] = minute() / 10;
  TimeDisp[3] = minute() % 10;
  tm1637.display(TimeDisp); 
}

#define TIME_HEADER  "T"   // Header tag for serial time sync message
unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 
  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
  }
  return pctime;
}
