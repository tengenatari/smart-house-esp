#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// Pin config
#define SS_PIN  5
#define RST_PIN 21
#define SERVO_PIN 14



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

// Door staet
int STATE = 0;

// Local cards list
#define MAX_ALLOWED_CARDS 16
char allowed_cards[MAX_ALLOWED_CARDS][13]; 
int allowed_cards_count = 0;

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
    Serial.println("Local card list full!");
    return false;
  }
  
  uid.toCharArray(allowed_cards[allowed_cards_count], 13);
  allowed_cards_count++;
  
  Serial.print("Added UID to local list: ");
  Serial.println(uid);
  return true;
}


void rfid_setup()
{
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("rfid ready");
}

void connect_to_wifi() {
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(100);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect!");
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
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("wifi problem");
    return -1;

  }
  
  HTTPClient http;

  String url = String(SERVER) + API + DEVICE_NAME;
  // Serial.println(url);

  http.begin(url);
  http.setTimeout(SERVER_TIMEOUT);

  http.addHeader("Content-Type", "application/json");

  String requestBody;
  serializeJson(requestDocument(), requestBody);
  
  Serial.print("Request body: ");
  Serial.println(requestBody);

  int httpCode = http.POST(requestBody);
  
  if (httpCode != 200) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    http.end();
    return -1;
  }

  String payload = http.getString();
  if (uid.length() == 0) return 2;
  /*
    {
      "metadata" = {
        "authorized_keys" = [
          "01:02:03:04",
          ...
        ];
        }
    }
  */
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  Serial.println(payload);
  
  http.end();
  if (!doc["metadata"].containsKey("authorized_cards") || !doc["metadata"]["authorized_cards"].is<JsonArray>()) {
    Serial.println("help");
    return -1;
  }
  return checkMulti(uid,doc) ? 1 : 0;
}


void addBlueCard() {
  String uid = "01:02:03:04";
  addUidToLocalList(uid);
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


String rfid_read()
{
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(10);
    return "";
  }

  String uid = formatUID(rfid.uid.uidByte, rfid.uid.size);
  Serial.print("detected UID:");
  Serial.println(uid);

  rfid.PICC_HaltA();
  return uid;
}

void openDoor() {
  Serial.println("Access granted");
  door_servo.write(90);
  STATE = 1;
}

void closeDoor() {
  door_servo.write(0);
  Serial.println("Door closed");
  STATE = 0;
}

void setup() {
  Serial.begin(115200);
  
  connect_to_wifi();
  door_servo.attach(SERVO_PIN);
  // closed
  door_servo.write(0);  
  rfid_setup();
  // addBlueCard();
  
}

void changeState() {
  if (STATE) closeDoor();
  else openDoor();
}

void auth(int authResult, String uid) {  
  if (authResult == 1) {
    addUidToLocalList(uid);
    Serial.println("Access Granted");
    changeState();
  } 
  else if (authResult == 0) {
    Serial.println("Access denied");
  }
  else {
    Serial.println("Server unreachable, checking local");
    if (checkLocalAccess(uid)) {
      Serial.println("Access Granted");
      changeState();
    } else {
      Serial.println("Access denied");
    }
  }
}

void loop() {
  delay(10);
  // Serial.println("Sanity check");
  String uid = rfid_read();
  
  int authResult = checkServerAccess(uid);
  if (authResult == 2 || uid.length() == 0) return; 
  auth(authResult,uid);
  
  delay(DELAY);
}