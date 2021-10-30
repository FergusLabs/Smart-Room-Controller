/*
   project: Sunrise w/hue
   name:Casey
*/

#include <TimeLib.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "TM1637.h"
#include <Encoder.h>
#include <OneButton.h>
#include <wemo.h>
#include <SPI.h>
#include <Ethernet.h>
#include <mac.h>
#include <hue.h>

#define ON 1
#define OFF 0
#define SCREEN_WIDTH 128    //constants for OLED
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

#define CLK 20 //pins for 4 digit display
#define DIO 21

int alarmHours = 14;      // THIS IS WHERE I SET THE ALARM TIME! 24 HOUR CLOCK. 
int alarmMinutes = 48;    // I would like to set the alarm with a button, but I did not have a chance to code it yet.

const int pinA = 7, pinB = 8, encButtonPin = 14, wemoButtonPin =2, timeSetPin = 3;   //pins for encoder and buttons

OneButton encButton (encButtonPin, INPUT_PULLUP);
OneButton wemoButton (wemoButtonPin, INPUT_PULLUP);
// OneButton timeSetButton(timeSetPin, INPUT_PULLUP); I didn't end up using this button.
Encoder myEnc(pinA, pinB);
TM1637 tm1637(CLK, DIO);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

const int pirPin = 1, tonePin = 23;
const int toneFreq = 528;
bool motionSeen, mode, audioOn, sunRiseOn, wemoOn;
int encPstn;  //encoder position
int hueSelect;
int wemoCoffee = 2, wemoTV = 1;
 
unsigned long currentTime, lastSecond, sunRiseTimer, sunRiseElapsed;

int8_t TimeDisp[] = {0x00, 0x00, 0x00, 0x00};
unsigned char ClockPoint = 1;

int beginSunRiseMinutes, sunRiseHour, sunRiseMinutes;
int earlyLUX, lateLUX, manLUX;

void setup() {
  setSyncProvider(getTeensy3Time);
  tm1637.set();
  tm1637.init();
  pinMode(pirPin, INPUT);
  pinMode(tonePin, OUTPUT);

  encButton.attachClick(click1);
  encButton.setClickTicks(250);
  wemoButton.attachClick(click2);
  wemoButton.setClickTicks(250);
  
  /*  
  I started attaching code for another button that would be used to set the alarm time. Perhaps in version 2.
  
  timeSetButton.attachClick(click3);
  timeSetButton.attachDoubleClick(dblclick);
  timeSetButton.attachLongPressStart(pressStart);
  timeSetButton.attachLongPressStop(pressStop);
  timeSetButton.setClickTicks(250);
  timeSetButton.setPressTicks(2000);
   */

  Serial.begin(9600);
  while (!Serial);
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.printf("SSD1306 allocation failed");
    for (;;);
  }
  Ethernet.begin(mac);
  delay(200);          //ensure Serial Monitor is up and running
  printIP();
  Serial.printf("LinkStatus: %i  \n", Ethernet.linkStatus());

  display.display();
  display.clearDisplay();
  display.display();

  for (hueSelect = 0; hueSelect < 6; hueSelect++) {
    setHue(hueSelect, false, HueYellow, manLUX, 0);
  }

  sunRiseOn = false;
  mode = false;
  audioOn = true;
}

void loop() {
  encButton.tick();
  wemoButton.tick();
  //setTimeButton.tick();
  currentTime = millis();
  motionSeen = digitalRead(pirPin);
  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
  if ((currentTime - lastSecond) > 1000) {
    OLEDClockDisplay();
    timeUpdate();
    lastSecond = millis();
  }
  motionDetect();
  manualMode();
  audioAlarm();
}

void manualMode (void) {
  if (mode) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 50);
    display.printf("You're up? \n Good Job. \n Everything is on manual.");
    display.display();
    sunRiseOn = false;
    
    encPstn = myEnc.read();
    if (encPstn > 95) {
      myEnc.write(96);
    }
    if (encPstn <= 0) {
      myEnc.write(0);
    }
    manLUX = map(encPstn, 0, 96, 0, 255);
    for (hueSelect = 0; hueSelect < 6; hueSelect++) {
      setHue(hueSelect, true, HueYellow, manLUX, 0);
    }
  }
  if (!mode) {
    sunRiseLite();
  }
}

void sunRiseLite (void) {
  beginSunRiseMinutes = ((alarmMinutes) - 10);
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
    switchON (wemoCoffee);
    sunRiseElapsed = ((currentTime) - (sunRiseTimer));
    if (sunRiseElapsed < 420000) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.printf("GOOD MORNING SUNSHINE!");
      display.display();
      earlyLUX = map(sunRiseElapsed, 0, 420000, 1, 55);
      for (hueSelect = 0; hueSelect < 6; hueSelect++) {
        setHue(hueSelect, true, HueViolet, earlyLUX, 255);
      }
    }
    if (sunRiseElapsed > 420000) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 50);
      display.printf("GET OUT OF BED!");
      display.display();
      lateLUX = map(sunRiseElapsed, 420000, 600000, 10, 255);
      for (hueSelect = 0; hueSelect < 6; hueSelect++) {
        setHue(hueSelect, true, HueYellow, lateLUX, 0);
      }
    }
  }
}

void click1 (void) {  //encoder button click
  mode = !mode;
}
void click2 (void) {  //
  wemoOn = !wemoOn;
}

void outletControl (void) {
  if (wemoOn) {  
    switchON (wemoCoffee);
    switchON (wemoTV);
  }
  if (!wemoOn) {
    switchOFF (wemoCoffee);
    switchOFF (wemoTV);
  }
}

void motionDetect (void) {
  if (motionSeen) {
    if (audioOn) {
      audioOn = false;
    }
    if (!mode) {
      mode = true;
    }
  }
}

void audioAlarm (void) { 
  if (audioOn) {
    
    if ((alarmHours == hour()) && (alarmMinutes == minute())) {
      tone(tonePin, toneFreq);
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 50);
      display.printf("GET UP! I WARNED YOU.");
      display.startscrollleft(0x00, 0x0F);
      display.display();;
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
  display.setCursor(0, 50);
  display.printf("%i", hour());
  OLEDprintDigits(minute());
  OLEDprintDigits(second());
  display.printf(" ");
  display.printf("%i", day());
  display.printf(" ");
  display.printf("%i", month());
  display.printf(" ");
  display.printf("%i", year());
  display.printf("");
  display.display();
}

void OLEDprintDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  display.printf(":");
  if (digits < 10)
    display.printf("0");
  display.printf("%i", digits);
}


void timeUpdate(void) { //time display to 4_Digit_Display
  TimeDisp[0] = hour() / 10;
  TimeDisp[1] = hour() % 10;
  TimeDisp[2] = minute() / 10;
  TimeDisp[3] = minute() % 10;
  tm1637.display(TimeDisp);
}

#define TIME_HEADER  "T"   // function to get time from PC
unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    if ( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L; // return 0 to indicate that the time is not valid
    }
  }
  return pctime;
}

void printIP() {    //function to get IP address
  Serial.printf("My IP address: ");
  for (byte thisByte = 0; thisByte < 3; thisByte++) {
    Serial.printf("%i.", Ethernet.localIP()[thisByte]);
  }
  Serial.printf("%i\n", Ethernet.localIP()[3]);
}
