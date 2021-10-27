/*
 * project: Sunrise w/neoPIX
 * name:Casey
 */

#include <TimeLib.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "TM1637.h"
#include <Encoder.h>
#include <OneButton.h>

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

const int pinA = 5, pinB = 6,buttonPin = 16;

OneButton encButton (buttonPin, INPUT_PULLUP);
Encoder myEnc(pinA, pinB);
Adafruit_NeoPixel pixel(PIXELCOUNT, PIXELPIN, NEO_GRB + NEO_KHZ800);
TM1637 tm1637(CLK,DIO);
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT, &Wire);
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

const int pirPin = 4,tonePin = 23;
const int toneFreq = 528;
bool motionDetect, mode, audioOn;
int encPstn;  //encoder position

unsigned long currentTime, lastSecond, sunRiseTimer;

int alarmHours = 23; 
int alarmMinutes = 10;

int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
unsigned char ClockPoint = 1;

bool sunRiseOn, prevSunState;
unsigned long sunRiseElapsed;
int beginSunRiseMinutes, sunRiseHour,sunRiseMinutes;
int earlyLUX, lateLUX, manLUX;

void setup() {
  
  setSyncProvider(getTeensy3Time);
  tm1637.set();
  tm1637.init();
  pinMode(pirPin, INPUT);
  pinMode(tonePin, OUTPUT);

  encButton.attachClick(click1);
  encButton.setClickTicks(250);
  
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
  pixel.clear();
  pixel.show();
  
  sunRiseOn = false;
  mode = false;
  audioOn = true;
}

void loop() {
  encButton.tick();
   currentTime = millis();
   motionDetect = digitalRead(pirPin);   
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
    Serial.printf("sunRiseOn=%i,mode=%i,audioOn=%i,motion=%i\n",sunRiseOn,mode,audioOn,motionDetect);
    
  }
  manualMode();
  audioAlarm();
}

void manualMode (void) {
  if (mode) {
  sunRiseOn = false;  
  encPstn = myEnc.read();
    if (encPstn > 95) {
      myEnc.write(96);
    }
    if (encPstn <= 0) {
      myEnc.write(0);
    }
  manLUX = map(encPstn,0,96,0,255);
  pixel.setPixelColor(0, yellow);
  pixel.setBrightness(manLUX);
  pixel.show();  
  }
  if (!mode) {
    sunRiseLite();
  }
}

void sunRiseLite (void) {
  
  beginSunRiseMinutes = ((alarmMinutes)-10);
  if (beginSunRiseMinutes >= 0) {
    if ((alarmHours == hour()) && (beginSunRiseMinutes == minute())) {
      if (!sunRiseOn) {
        sunRiseOn = true;
        sunRiseTimer = millis();        
      }
    }
  }  
  if (beginSunRiseMinutes < 0) {
    sunRiseHour = ((alarmHours) - 1);
    sunRiseMinutes = (beginSunRiseMinutes + 60); 
      if ((sunRiseHour == hour()) && (sunRiseMinutes == minute())) {
        if (!sunRiseOn) {
          sunRiseOn = true;
          sunRiseTimer = millis();        
        }
      }      
  }  
  if (sunRiseOn) { 
     sunRiseElapsed = ((currentTime)-(sunRiseTimer));
     if (sunRiseElapsed < 420000) {
        earlyLUX = map(sunRiseElapsed,0,420000,1,35);
        pixel.setPixelColor(0, blue);
        pixel.setBrightness(earlyLUX);
        pixel.show();
     }
     if (sunRiseElapsed > 420000) {
        lateLUX = map(sunRiseElapsed,420000,600000,1,255);
        pixel.setPixelColor(0, yellow);
        pixel.setBrightness(lateLUX);
        pixel.show();
     }
  }
}

void click1 (void) {
  mode = !mode;
}
void audioAlarm (void) {
  if (motionDetect) {
    if (audioOn) {
      audioOn = !audioOn;
    }
  }
  if (audioOn) {
    if ((alarmHours == hour()) && (alarmMinutes == minute())) {
      tone(tonePin, toneFreq);
    }
  } 
  if (!audioOn) {
    noTone(tonePin);
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
