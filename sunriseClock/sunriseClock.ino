/*
 * Project: Smart Room Controller
 * Description: Sunrise Alarm Clock with Smart Features
 * Name: Casey Fergus
 * Date 10/25/2021
 */

#include <TimeLib.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "TM1637.h"

#include <SPI.h>
#include <Ethernet.h>
#include <mac.h>
#include <hue.h>
#include <Encoder.h>
#include <OneButton.h>


#define ON 1
#define OFF 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define BME_ADDRESS 0x76

#define CLK 0 //pins for 4 digit display
#define DIO 1

const int tonePin = 2, toneFreq = 528, toneDur = 250;   //constants for audio alarm
//const int encPinA = 5, encPinB = 6, buttonPin = 16; // constants for manual operation
//int encPstn;
 

//Encoder myEnc(encPinB, encPinA);
OneButton encButton (buttonPin, INPUT_PULLUP);

TM1637 tm1637(CLK,DIO);
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT, &Wire);
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

int hueSelect = 3;
bool hueState;
int LUX; //brightness
int colorSelect;

unsigned long currentTime, lastSecond, sunRiseTimer;

int alarmHours = 10; 
int alarmMinutes = 50;

int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
unsigned char ClockPoint = 1;

void setup() {
  setSyncProvider(getTeensy3Time);
  tm1637.set();
  tm1637.init();

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  
  Serial.begin(9600);
  while(!Serial);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.printf("SSD1306 allocation failed");
    for(;;);
  }
  if (timeStatus()!= timeSet) {
     Serial.println("Unable to sync with the RTC");
  }

  Ethernet.begin(mac);
  delay(200);          //ensure Serial Monitor is up and running           
  printIP();
  //Serial.printf("LinkStatus: %i  \n",Ethernet.linkStatus());

  display.display();
  display.clearDisplay();
  display.display();
  
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
  audioAlarm();
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

void sunRiseLite (void) {
  unsigned long beginSunRiseMinutes,sunRiseTimer,sunRiseElapsed,sunRiseHour,sunRiseMinutes;
  int earlyLUX,lateLUX;
  bool sunRiseOnOff;
  beginSunRiseMinutes = ((alarmMinutes)-10);
  if (beginSunRiseMinutes >= 0) {
    if ((alarmHours == hour()) && (beginSunRiseMinutes == minute())) {
      sunRiseOn = !SunRiseOn
      if (sunRiseOn)
      Serial.printf("sunrise elapsed time =%i\n",sunRiseElapsed);
      sunRiseTimer = millis();
      sunRiseElapsed = ((currentTime)-(sunRiseTimer));
        if (sunRiseElapsed < 5000) {
          earlyLUX = map(sunRiseElapsed,0,5000,0,75);
          //setHue(hueSelect, true, HueViolet, earlyLUX, 255);
        }
        if (sunRiseElapsed > 5000) {
          lateLUX = map(sunRiseElapsed,5000,10000,75,255);
          //setHue(hueSelect, true, HueYellow, lateLUX, 255);
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
          setHue(hueSelect, true, HueViolet, earlyLUX, 255);
        }
        if (sunRiseElapsed > 5000) {
          lateLUX = map(sunRiseElapsed,5000,10000,75,255);
          setHue(hueSelect, true, HueYellow, lateLUX, 255);
        }
    }
  }
}

void audioAlarm (void) {
  if ((alarmHours == hour()) && (alarmMinutes == minute())) {
    tone(tonePin,toneFreq,toneDur);  
  }
}

void timeUpdate(void) {
  TimeDisp[0] = hour() / 10;
  TimeDisp[1] = hour() % 10;
  TimeDisp[2] = minute() / 10;
  TimeDisp[3] = minute() % 10;
  tm1637.display(TimeDisp); 
}




/* Below are functions to start/run components*/

void printIP() {
  Serial.printf("My IP address: ");
  for (byte thisByte = 0; thisByte < 3; thisByte++) {
    Serial.printf("%i.",Ethernet.localIP()[thisByte]);
  }
  Serial.printf("%i\n",Ethernet.localIP()[3]);
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
