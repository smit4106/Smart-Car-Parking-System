#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

// ---------------- WIFI ----------------
const char* ssid = "Galaxy M34 5G";
const char* password = "04012026";

// ---------------- RENDER SERVER ----------------
const char* serverURL = "https://smart-car-parking-system-0f2w.onrender.com/log";

// ---------------- NTP ----------------
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- ULTRASONIC ----------------
#define TRIG_PIN_1 5
#define ECHO_PIN_1 18
#define TRIG_PIN_2 19
#define ECHO_PIN_2 21
#define TRIG_PIN_3 22
#define ECHO_PIN_3 23

const int threshold = 10;
const int confirmCount = 2;

int slot1Count = 0, slot2Count = 0, slot3Count = 0;
bool slot1Occupied = false;
bool slot2Occupied = false;
bool slot3Occupied = false;

bool previousSlot1State = false;
bool previousSlot2State = false;
bool previousSlot3State = false;

unsigned long entryMillis1 = 0;
unsigned long entryMillis2 = 0;
unsigned long entryMillis3 = 0;

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  pinMode(TRIG_PIN_3, OUTPUT);
  pinMode(ECHO_PIN_3, INPUT);

  Wire.begin(25, 26);
  lcd.init();
  lcd.backlight();

  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected");
  Serial.println("WiFi Connected");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // 🔥 WAIT FOR REAL TIME SYNC
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for NTP time...");
    delay(1000);
  }
  Serial.println("Time synchronized!");

  delay(1500);
  lcd.clear();
}

// ---------------- LOOP ----------------
void loop() {

  int d1 = getDistance(TRIG_PIN_1, ECHO_PIN_1);
  delay(50);
  int d2 = getDistance(TRIG_PIN_2, ECHO_PIN_2);
  delay(50);
  int d3 = getDistance(TRIG_PIN_3, ECHO_PIN_3);

  Serial.print("D1: "); Serial.print(d1);
  Serial.print("  D2: "); Serial.print(d2);
  Serial.print("  D3: "); Serial.println(d3);

  updateSlot(d1, slot1Count, slot1Occupied);
  updateSlot(d2, slot2Count, slot2Occupied);
  updateSlot(d3, slot3Count, slot3Occupied);

  // -------- SLOT 1 --------
  if (slot1Occupied && !previousSlot1State) {
    entryMillis1 = millis();
    Serial.println("Vehicle Entered Slot 1");
  }

  if (!slot1Occupied && previousSlot1State) {
    unsigned long duration = (millis() - entryMillis1) / 1000;
    Serial.println("Vehicle Exited Slot 1");
    sendToServer(1, getTimeFromMillis(entryMillis1), getCurrentTime(), duration);
  }

  // -------- SLOT 2 --------
  if (slot2Occupied && !previousSlot2State) {
    entryMillis2 = millis();
    Serial.println("Vehicle Entered Slot 2");
  }

  if (!slot2Occupied && previousSlot2State) {
    unsigned long duration = (millis() - entryMillis2) / 1000;
    Serial.println("Vehicle Exited Slot 2");
    sendToServer(2, getTimeFromMillis(entryMillis2), getCurrentTime(), duration);
  }

  // -------- SLOT 3 --------
  if (slot3Occupied && !previousSlot3State) {
    entryMillis3 = millis();
    Serial.println("Vehicle Entered Slot 3");
  }

  if (!slot3Occupied && previousSlot3State) {
    unsigned long duration = (millis() - entryMillis3) / 1000;
    Serial.println("Vehicle Exited Slot 3");
    sendToServer(3, getTimeFromMillis(entryMillis3), getCurrentTime(), duration);
  }

  previousSlot1State = slot1Occupied;
  previousSlot2State = slot2Occupied;
  previousSlot3State = slot3Occupied;

  displayLCD();

  delay(300);
}

// ---------------- ULTRASONIC ----------------
int getDistance(int trigPin, int echoPin) {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0) return 400;

  int distance = duration * 0.034 / 2;
  if (distance < 0 || distance > 400) return 400;

  return distance;
}

// ---------------- SLOT LOGIC ----------------
void updateSlot(int distance, int &counter, bool &occupied) {

  if (distance < threshold) {
    if (counter < confirmCount) counter++;
  } else {
    if (counter > 0) counter--;
  }

  occupied = (counter >= confirmCount);
}

// ---------------- LCD ----------------
void displayLCD() {

  int freeSlots = 0;
  if (!slot1Occupied) freeSlots++;
  if (!slot2Occupied) freeSlots++;
  if (!slot3Occupied) freeSlots++;

  lcd.clear();

  if (freeSlots == 0) {
    lcd.setCursor(0,0);
    lcd.print(" PARKING FULL ");
    lcd.setCursor(0,1);
    lcd.print(" NO SLOT FREE ");
  } 
  else {
    lcd.setCursor(0,0);
    lcd.print("Free Slots: ");
    lcd.print(freeSlots);

    lcd.setCursor(0,1);
    lcd.print("S1:");
    lcd.print(slot1Occupied ? "F " : "E ");
    lcd.print("S2:");
    lcd.print(slot2Occupied ? "F " : "E ");
    lcd.print("S3:");
    lcd.print(slot3Occupied ? "F" : "E");
  }
}

// ---------------- TIME ----------------
String getCurrentTime() {

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return "0000-00-00 00:00:00";

  char buffer[25];
  strftime(buffer, sizeof(buffer),
           "%Y-%m-%d %H:%M:%S", &timeinfo);

  return String(buffer);
}

String getTimeFromMillis(unsigned long pastMillis) {

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  time_t now = mktime(&timeinfo);
  time_t entryTime = now - ((millis() - pastMillis) / 1000);

  struct tm* entryTm = localtime(&entryTime);

  char buffer[25];
  strftime(buffer, sizeof(buffer),
           "%Y-%m-%d %H:%M:%S", entryTm);

  return String(buffer);
}

// ---------------- SEND TO SERVER ----------------
void sendToServer(int slot,
                  String entryTime,
                  String exitTime,
                  unsigned long duration) {

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.setTimeout(3000);   // 🔥 Prevent freezing

    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{";
    jsonData += "\"slot\":" + String(slot) + ",";
    jsonData += "\"entry_time\":\"" + entryTime + "\",";
    jsonData += "\"exit_time\":\"" + exitTime + "\",";
    jsonData += "\"duration_seconds\":" + String(duration);
    jsonData += "}";

    Serial.println("Sending JSON:");
    Serial.println(jsonData);

    int response = http.POST(jsonData);

    Serial.print("Server Response: ");
    Serial.println(response);

    http.end();
  }
}
