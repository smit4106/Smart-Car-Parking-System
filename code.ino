#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <ESP32Servo.h>
#include "time.h"

// ---------------- WIFI ----------------
const char* ssid = "Wifi-Name";
const char* password = "Password";
const char* serverURL = "URL of cloud";

// ---------------- NTP TIME ----------------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

// ---------------- I2C BUSES ----------------
TwoWire I2C_Entry = TwoWire(0);
TwoWire I2C_Exit  = TwoWire(1);

LiquidCrystal_PCF8574 entryLcd(0x27);
LiquidCrystal_PCF8574 exitLcd(0x27);

// ---------------- SERVOS ----------------
Servo entryServo;
Servo exitServo;

#define ENTRY_SERVO_PIN 13
#define EXIT_SERVO_PIN 12

// ---------------- SLOT SENSORS ----------------
#define TRIG1 5
#define ECHO1 18
#define TRIG2 19
#define ECHO2 21
#define TRIG3 22
#define ECHO3 23

// ---------------- GATE SENSORS ----------------
#define ENTRY_TRIG 32
#define ENTRY_ECHO 33
#define EXIT_TRIG 4
#define EXIT_ECHO 2

const int slotThreshold = 5;
const int gateThreshold = 5;

bool slot1=false, slot2=false, slot3=false;
bool prev1=false, prev2=false, prev3=false;

unsigned long entryTime1=0, entryTime2=0, entryTime3=0;

int lastExitedSlot = 0;
unsigned long lastDuration = 0;
unsigned long lastCharge = 0;

void setup() {

  Serial.begin(115200);

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);
  pinMode(ENTRY_TRIG, OUTPUT); pinMode(ENTRY_ECHO, INPUT);
  pinMode(EXIT_TRIG, OUTPUT);  pinMode(EXIT_ECHO, INPUT);

  entryServo.attach(ENTRY_SERVO_PIN);
  exitServo.attach(EXIT_SERVO_PIN);

  entryServo.write(0);
  exitServo.write(0);

  I2C_Entry.begin(25, 26);
  I2C_Exit.begin(27, 14);

  entryLcd.begin(20,4,I2C_Entry);
  exitLcd.begin(16,2,I2C_Exit);

  entryLcd.setBacklight(255);
  exitLcd.setBacklight(255);

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED) delay(500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {

  updateSlots();          // 🔴 Update slots first
  handleEntryGate();
  handleExitGate();
  displayEntryLCD();

  delay(300);
}

// ---------------- ENTRY GATE ----------------
void handleEntryGate(){

  static bool gateOpen = false;
  static unsigned long carLeftTime = 0;

  int available = (!slot1) + (!slot2) + (!slot3);
  int d = getDistance(ENTRY_TRIG,ENTRY_ECHO);

  // 🚫 If parking full → keep gate closed
  if(available == 0){
    entryServo.write(0);
    gateOpen = false;
    return;
  }

  // ✅ Normal operation if space available
  if(d < gateThreshold){
    entryServo.write(90);
    gateOpen = true;
    carLeftTime = 0;
  }
  else{
    if(gateOpen){
      if(carLeftTime == 0){
        carLeftTime = millis();
      }

      if(millis() - carLeftTime >= 3000){
        entryServo.write(0);
        gateOpen = false;
        carLeftTime = 0;
      }
    }
  }
}

// ---------------- EXIT GATE ----------------
void handleExitGate(){

  static bool processingExit = false;

  int d = getDistance(EXIT_TRIG,EXIT_ECHO);

  if(d < gateThreshold && lastExitedSlot != 0 && !processingExit){

    processingExit = true;

    exitLcd.clear();
    exitLcd.setCursor(0,0);
    exitLcd.print("Charge: Rs ");
    exitLcd.print(lastCharge);

    delay(5000);

    exitServo.write(90);
    delay(500);

    exitLcd.clear();
    exitLcd.setCursor(3,0);
    exitLcd.print("THANK YOU");

    delay(4000);

    exitServo.write(0);
    delay(500);

    exitLcd.clear();

    sendToServer(lastExitedSlot,0,0,lastDuration,lastCharge);

    lastExitedSlot = 0;
    processingExit = false;
  }
}

// ---------------- SLOT UPDATE ----------------
void updateSlots(){

  int d1=getDistance(TRIG1,ECHO1);
  int d2=getDistance(TRIG2,ECHO2);
  int d3=getDistance(TRIG3,ECHO3);

  slot1 = d1 < slotThreshold;
  slot2 = d2 < slotThreshold;
  slot3 = d3 < slotThreshold;

  if(!slot1 && prev1){
    lastExitedSlot = 1;
    lastDuration = (millis() - entryTime1)/1000;
    lastCharge = lastDuration;
  }
  else if(!slot2 && prev2){
    lastExitedSlot = 2;
    lastDuration = (millis() - entryTime2)/1000;
    lastCharge = lastDuration;
  }
  else if(!slot3 && prev3){
    lastExitedSlot = 3;
    lastDuration = (millis() - entryTime3)/1000;
    lastCharge = lastDuration;
  }

  if(slot1 && !prev1) entryTime1=millis();
  if(slot2 && !prev2) entryTime2=millis();
  if(slot3 && !prev3) entryTime3=millis();

  prev1=slot1;
  prev2=slot2;
  prev3=slot3;
}

// ---------------- ENTRY LCD ----------------
void displayEntryLCD(){

  int available = (!slot1)+(!slot2)+(!slot3);

  entryLcd.clear();

  if(available == 0){
    entryLcd.setCursor(3,1);
    entryLcd.print("PARKING FULL");
    return;
  }

  entryLcd.setCursor(0,0);
  entryLcd.print("PARKING SYSTEM");

  entryLcd.setCursor(0,1);
  entryLcd.print("Available: ");
  entryLcd.print(available);

  entryLcd.setCursor(0,2);
  entryLcd.print("S1:");
  entryLcd.print(slot1?"FULL ":"EMPTY");

  entryLcd.setCursor(0,3);
  entryLcd.print("S2:");
  entryLcd.print(slot2?"FULL ":"EMPTY");
  entryLcd.print(" S3:");
  entryLcd.print(slot3?"FULL":"EMPTY");
}

// ---------------- DISTANCE FUNCTION ----------------
int getDistance(int t,int e){
  digitalWrite(t,LOW); delayMicroseconds(2);
  digitalWrite(t,HIGH); delayMicroseconds(10);
  digitalWrite(t,LOW);
  long duration=pulseIn(e,HIGH,30000);
  if(duration<=0) return 400;
  return duration*0.034/2;
}

// ---------------- SERVER ----------------
void sendToServer(int slot,
                  unsigned long entryTimeMillis,
                  unsigned long exitTimeMillis,
                  unsigned long duration,
                  unsigned long charge){

  if(WiFi.status()==WL_CONNECTED){

    struct tm timeinfo;
    time_t now;
    time(&now);

    char exitString[30];
    char entryString[30];

    localtime_r(&now, &timeinfo);
    strftime(exitString, sizeof(exitString), "%Y-%m-%d %H:%M:%S", &timeinfo);

    time_t entryReal = now - duration;
    struct tm entryInfo;
    localtime_r(&entryReal, &entryInfo);
    strftime(entryString, sizeof(entryString), "%Y-%m-%d %H:%M:%S", &entryInfo);

    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type","application/json");

    String json="{";
    json+="\"slot\":"+String(slot)+",";
    json+="\"entry_time\":\""+String(entryString)+"\",";
    json+="\"exit_time\":\""+String(exitString)+"\",";
    json+="\"duration_sec\":"+String(duration)+",";
    json+="\"charge\":"+String(charge);
    json+="}";

    http.POST(json);
    http.end();
  }
}
