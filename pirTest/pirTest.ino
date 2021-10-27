/*
 * project: PIR test
 * name:Casey
 */

int pirPin = 4;
bool motionDetect;
unsigned long currentTime, lastSecond;

void setup() {
  pinMode(pirPin, INPUT);
  Serial.begin(9600);
  while(!Serial);
  
}

void loop() {
  currentTime = millis();
  motionDetect = digitalRead(pirPin);
  if ((currentTime - lastSecond)>1000) {
    lastSecond = millis();
    Serial.printf("motion=%i\n",motionDetect);
  }  
}
