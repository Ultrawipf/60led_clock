#include <Wire.h> 
#include "RTClib.h"  
#include <OneWire.h>            
#include <DallasTemperature.h>  
#include <FastLED.h>

#define NUM_LEDS 60 //obviously...
#define DATA_PIN 6 //for leds
#define DS18B20_PIN 5   // Pin for DS18B20
#define OFFSET 30
#define TIMEZONE 2 //timezone offset for unix times

RTC_DS1307 RTC; 
bool timeUpdate=false;
int lastBrightness;
CRGB leds[NUM_LEDS];
CRGB tempLeds[NUM_LEDS]; //Second buffer for animations or storage
OneWire oneWire(DS18B20_PIN); 
DallasTemperature sensors(&oneWire);


void rotateAnim(int del,int times){
  uint8_t hue = 0;
  for(int i=0;i<times;i++){
    for(int dot = 0; dot < NUM_LEDS; dot++) { 
              leds[o(dot)] = CHSV(hue+=2, 255, 255);
              FastLED.show();
              // clear this led for the next time around the loop
              delay(del);
    }
  }
  //reset
  for(int led = 0; led < NUM_LEDS; led++) {
    leds[led]=CRGB::Black;
  }
   FastLED.show();
}

//Calculate offset for direct array access
int o(int num){
  return (num + OFFSET) % NUM_LEDS;
}

//----------Time Display-------------------
void showTime(DateTime curtime,CRGB* buf){
  //clear all leds
  for(int led = 0; led < NUM_LEDS; led++) {
    if(led%15==0 && FastLED.getBrightness()>8)
      buf[led]=CRGB(30,30,30);
    else
      buf[led]=CRGB::Black;
  }
  int curhour=((curtime.hour()%12) * 5 )+map(curtime.minute(),0,59,0,5);
  buf[o(curhour)].b=255;
  buf[o(curtime.minute())].g=255;
  buf[o(curtime.second())].r=200;
  FastLED.show();
}


//--------------Temperature Display--------------
void showTemperature(){
  for(int led = 0; led < NUM_LEDS; led++) {
    leds[led]=CRGB::Black;
  }
  sensors.requestTemperatures();                 // Read temperature
  float temperature = sensors.getTempCByIndex(0);  
  int temp = min(max(round(temperature),0),59);
  for(int led = 0; led <= temp; led++) {
    CRGB color;
    if(led<10)
      color = CRGB::Blue;
    else if(led<20)
      color = CRGB::Purple;
    else if(led<25)
      color = CRGB::Green;
    else
      color = CRGB::Red;

    //If n*10 set to white
    if(led%10 == 0)
      color = CRGB::White;
       
    leds[o(led)]= color / (led==temp ? 1 : 2); //division because of strange * operator behaviour
   
    FastLED.show();
    delay(10);
  }
  delay(1800);
  //Return animation
  DateTime now=RTC.now();
  showTime(now,tempLeds);
  for(int led = 0; led <= NUM_LEDS; led++) {
    leds[o(led)]=tempLeds[o(led)];
    FastLED.show();
    delay(10);
  }
  
}


void updateTime(){
  DateTime now=RTC.now();
  int sec = now.second();
  showTime(now, leds);
  if(sec==59 && now.hour()==13 && now.minute()==36){
    rotateAnim(10,20);
  }else if(sec==59 && FastLED.getBrightness()>8)
    showTemperature();

  
}

void rtc_isr(){
  timeUpdate=true;
}

void setup() {
  Wire.begin();
  RTC.begin();
  RTC.writeSqwPinMode(SquareWave1HZ);
  FastLED.addLeds<WS2812B, DATA_PIN,GRB>(leds, NUM_LEDS);
  rotateAnim(8,3);
  
  //leds.fadeToBlackBy(40);
  if (! RTC.isrunning()) {
    
    //Initial clock set.
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  pinMode(2,INPUT_PULLUP);
  attachInterrupt(0, rtc_isr, RISING);
  sensors.begin();  // DS18B20 starten
  FastLED.setBrightness(128);
  lastBrightness= FastLED.getBrightness();
  Serial.begin(9600);
}

void loop() {
  if(timeUpdate && FastLED.getBrightness()>0){
    updateTime();
    timeUpdate=false;
  }
  int brightness=map(min(max(analogRead(0),25),900),25,900,0,255); //Brightness range
  if(abs(brightness-lastBrightness)>4){
    
    lastBrightness=brightness;
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(20);
  }
  if(Serial.available()>0){
    char c = Serial.read();
    if(c=='s'){
      //Set Time ex. "s1463068880" unix Timestamp
      uint32_t utime = Serial.parseInt();
      
      DateTime newTime(utime);
      DateTime newTimeTZ = newTime + TimeSpan(0,TIMEZONE,0,0); //Timezone +2h
      RTC.adjust(newTimeTZ);
     }
     if(c=='r'){
        DateTime now=RTC.now();
        Serial.println((now -  TimeSpan(0,TIMEZONE,0,0)).unixtime());
        Serial.println(String(now.hour())+":"+String(now.minute())+":"+String(now.second())+ " "+String(now.day())+" "+String(now.month())+" "+String(now.year()));
     }
  }
  
  delay(10);
}
