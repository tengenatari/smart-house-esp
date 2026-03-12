#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <SD.h>
#include <FS.h>

// Pin config
#define SS_PIN  5
#define RST_PIN 21
#define SERVO_PIN 14

#define CS_PIN 32  // SD card Chip Select pin

// Wifi config
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

// Server config
#define SERVER "http://iversy.ru:8751"
#define DEVICE_NAME "DOOR-1"
#define API "/api/v1/heartbeat/"
#define TOKEN "walkieneelitalkie"
#define DELAY 2000
#define SERVER_TIMEOUT 10000

// File paths on SD card
#define AUTH_FILE "/authorized_cards.txt"
#define LOG_FILE "/log.txt"

// Door state
int STATE = 0;

// Local cards list
#define MAX_ALLOWED_CARDS 16
char allowed_cards[MAX_ALLOWED_CARDS][13]; 
int allowed_cards_count = 0;

// SD log file handle (global for persistent logging)
File logFile;

StaticJsonDocument<100> requestDocument() {
  StaticJsonDocument<100> values;
  values["state"] = STATE;

  StaticJsonDocument<100> doc;
  doc["token"] = TOKEN;
  doc["values"] = values;
  return doc;
}

MFRC522 rfid(SS_PIN, RST_PIN);
Servo door_servo;

void logPrint(const String &s) {
  Serial.print(s);
  if (logFile) logFile.print(s);
}

void logPrintln(const String &s) {
  Serial.println(s);
  if (logFile) logFile.println(s);
}

void logPrint(const char* s) {
  Serial.print(s);
  if (logFile) logFile.print(s);
}

void logPrintln(const char* s) {
  Serial.println(s);
  if (logFile) logFile.println(s);
}

void logPrintf(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  Serial.print(buf);
  if (logFile) logFile.print(buf);
}

bool initSD() {
  if (!SD.begin(CS_PIN)) {
    Serial.println("ERROR: SD card initialization failed! Check wiring.");
    return false;
  }
  Serial.println("SD card initialized.");
  
  logFile = SD.open(LOG_FILE, FILE_APPEND);
  if (!logFile) {
    Serial.println("WARNING: Could not open log file for writing");
    return false;
  }
  logPrintln("\nSystem Started");
  return true;
}

bool loadAuthorizedCards() {
  if (!SD.exists(AUTH_FILE)) {
    logPrintln("No authorized cards file found, starting with empty list");
    return false;
  }
  
  File file = SD.open(AUTH_FILE, FILE_READ);
  if (!file) {
    logPrintln("ERROR: Failed to open " AUTH_FILE " for reading");
    return false;
  }
  
  allowed_cards_count = 0;
  while (file.available() && allowed_cards_count < MAX_ALLOWED_CARDS) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && line.length() < 13) {
      line.toCharArray(allowed_cards[allowed_cards_count], 13);
      allowed_cards_count++;
    }
  }
  file.close();
  logPrintf("Loaded %d authorized cards from file\n", allowed_cards_count);
  return true;
}

bool saveAuthorizedCards() {
  File file = SD.open(AUTH_FILE, FILE_WRITE);
  if (!file) {
    logPrintln("ERROR: Failed to open " AUTH_FILE " for writing");
    return false;
  }
  
  for (int i = 0; i < allowed_cards_count; i++) {
    file.println(allowed_cards[i]);
  }
  file.close();
  logPrintf("Saved %d authorized cards to file\n", allowed_cards_count);
  return true;
}

bool checkLocalAccess(String uid) {
  for (int i = 0; i < allowed_cards_count; i++) {
    if (uid.equalsIgnoreCase(allowed_cards[i])) {
      return true;
    }
  }
  return false;
}

bool addUidToLocalList(String uid) {
  if (checkLocalAccess(uid)) return true;
  
  if (allowed_cards_count >= MAX_ALLOWED_CARDS) {
    logPrintln("Local card list full!");
    return false;
  }
  
  uid.toCharArray(allowed_cards[allowed_cards_count], 13);
  allowed_cards_count++;
  
  logPrint("Added UID to local list: ");
  logPrintln(uid);
  
  saveAuthorizedCards();
  return true;
}

void rfid_setup() {
  SPI.begin();
  rfid.PCD_Init();
  logPrintln("RFID reader ready");
}

String formatUID(byte *uid, byte size) {
  String result = "";
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
    if (i < size - 1) result += ":";
  }
  result.toUpperCase();
  return result;
}

String rfid_read() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(10);
    return "";
  }

  String uid = formatUID(rfid.uid.uidByte, rfid.uid.size);
  logPrint("Detected UID: ");
  logPrintln(uid);

  rfid.PICC_HaltA();
  return uid;
}

void connect_to_wifi() {
  logPrint("Connecting to WiFi ");
  logPrintln(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(100);
    logPrint(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    logPrintln("Connected!");
    logPrint("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    logPrintln("Failed to connect!");
  }
  
  delay(1000);
}

bool checkMulti(String uid, StaticJsonDocument<200> doc) {
  JsonArray authorizedCards = doc["metadata"]["authorized_cards"].as<JsonArray>();

  for (JsonVariant card : authorizedCards) {
    if (card.is<String>() && card.as<String>() == uid) {
      return true; 
    }
  }
  return false;
}

int checkServerAccess(String uid) {
  if (WiFi.status() != WL_CONNECTED) {
    logPrintln("WiFi problem");
    return -1;
  }
  
  HTTPClient http;
  String url = String(SERVER) + API + DEVICE_NAME;

  http.begin(url);
  http.setTimeout(SERVER_TIMEOUT);
  http.addHeader("Content-Type", "application/json");

  String requestBody;
  serializeJson(requestDocument(), requestBody);
  
  logPrint("Request body: ");
  logPrintln(requestBody);

  int httpCode = http.POST(requestBody);
  
  if (httpCode != 200) {
    logPrintf("HTTP Response code: %d\n", httpCode);
    http.end();
    return -1;
  }

  String payload = http.getString();
  if (uid.length() == 0) return 2;
  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  logPrintln(payload);
  
  http.end();
  if (!doc["metadata"].containsKey("authorized_cards") || !doc["metadata"]["authorized_cards"].is<JsonArray>()) {
    logPrintln("ERROR: Invalid server response format");
    return -1;
  }
  return checkMulti(uid,doc) ? 1 : 0;
}

void openDoor() {
  logPrintln("Access granted - Opening door");
  door_servo.write(90);
  STATE = 1;
}

void closeDoor() {
  door_servo.write(0);
  logPrintln("Door closed");
  STATE = 0;
}

void changeState() {
  if (STATE) closeDoor();
  else openDoor();
}

void auth(int authResult, String uid) {  
  if (authResult == 1) {
    addUidToLocalList(uid);
    logPrintln("Access Granted");
    changeState();
  } 
  else if (authResult == 0) {
    logPrintln("Access denied");
  }
  else {
    logPrintln("Server unreachable, checking local list");
    if (checkLocalAccess(uid)) {
      logPrintln("Access Granted (local)");
      changeState();
    } else {
      logPrintln("Access denied");
    }
  }
}

void addBlueCard() {
  String uid = "01:02:03:04";
  addUidToLocalList(uid);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  logPrintln("\nstarting...");
  
  initSD();
  
  loadAuthorizedCards();
  
  connect_to_wifi();
  
  door_servo.attach(SERVO_PIN);
  closeDoor(); // Start with door closed
  
  rfid_setup();
  // addBlueCard();
  
  logPrintln("System ready - Waiting for cards...");
}

void loop() {
  delay(10);
  String uid = rfid_read();
  int authResult = checkServerAccess(uid);
  
  if (authResult == 2 | uid.length() == 0) return; 
  
  
  auth(authResult, uid);
  
  delay(DELAY);
}