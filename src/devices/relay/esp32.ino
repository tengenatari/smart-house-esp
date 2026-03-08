#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define HEALTH_PIN 0
#define RELAY_PIN 4
#define SERVER "http://iversy.ru:8751"
#define DEVICE_NAME "RELAY-1"
#define TOKEN "abacaba"
#define DELAY 5000

boolean STATE = false;
boolean SUCCESS = false;

void connectToWiFi() {
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
    Serial.println(" Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" Failed to connect!");
  }
  
  Serial.println();
  delay(1000);
}

StaticJsonDocument<100> requestDocument() {
  StaticJsonDocument<100> values;
  values["state"] = int(STATE);
  
  StaticJsonDocument<100> doc;
  doc["token"] = TOKEN;
  doc["values"] = values;

  return doc;
}

void processResponse(HTTPClient &http, String payload) {
  StaticJsonDocument<100> responseDoc;
  DeserializationError error = deserializeJson(responseDoc, payload);
      
  if (error) {
    Serial.print("Failed to parse JSON response: ");
    Serial.println(error.c_str());
    http.end();
    return;
  }

  STATE = responseDoc["state"];
  SUCCESS = true;
  http.end();
}

void sendState() {
  SUCCESS = false;
  HTTPClient http;
  
  String url = String(SERVER) + "/api/v1/heartbeat/" + DEVICE_NAME;
  http.begin(url);
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");

  String requestBody;
  serializeJson(requestDocument(), requestBody);
  
  Serial.print("Request body: ");
  Serial.println(requestBody);

  int httpCode = http.POST(requestBody);
  
  if (httpCode != 200) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    http.end();
    return;
  }

  String payload = http.getString();
  Serial.print("Response: ");
  Serial.println(payload);
      
  processResponse(http, payload);
}

void setup(void) {
  Serial.begin(115200);

  pinMode(HEALTH_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  digitalWrite(RELAY_PIN, LOW);
  
  connectToWiFi();
}

void loop(void) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Reconnecting...");
    digitalWrite(HEALTH_PIN, LOW);
    connectToWiFi();
    delay(DELAY);
    return;
  }

  boolean currentRelayState = digitalRead(RELAY_PIN);
  sendState();
  
  digitalWrite(HEALTH_PIN, SUCCESS);
  if (!SUCCESS) {
    delay(DELAY);
    return;
  }
  
  if (STATE != currentRelayState) {
    digitalWrite(RELAY_PIN, STATE);
    Serial.printf("Relay state changed to: %s\n", STATE ? "ON" : "OFF");
  }
  delay(DELAY);
}
