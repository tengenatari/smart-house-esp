#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define HEALTH_PIN 0
#define RELAY_PIN 4
#define SERVER "http://109.194.163.31:8751"
#define DEVICE_ID ""
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

void sendRelayState() {
  SUCCESS = false;

  Serial.print("Sending HTTP request to ");
  Serial.print(SERVER);
  Serial.println("/api/v1/" DEVICE_ID);

  HTTPClient http;
  
  String url = String(SERVER) + "/api/v1/" DEVICE_ID;
  http.begin(url);
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["active"] = STATE;
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  Serial.print("Request body: ");
  Serial.println(requestBody);

  int httpCode = http.POST(requestBody);
  
  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.print("Response: ");
      Serial.println(payload);
      
      StaticJsonDocument<200> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, payload);
      
      if (!error) {
        STATE = responseDoc["active"];
        SUCCESS = true;
      } else {
        Serial.print("Failed to parse JSON response: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.printf("Error: Unexpected HTTP code: %d\n", httpCode);
    }
  } else {
    Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void setup(void) {
  Serial.begin(115200);
  
  // Настраиваем пины
  pinMode(HEALTH_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  // Начальное состояние реле - выключено
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

  sendRelayState();
  
  digitalWrite(HEALTH_PIN, SUCCESS);
  if (!SUCCESS) {
    delay(DELAY);
    return;
  }
    
  // Обновляем состояние реле если оно изменилось
  if (STATE != currentRelayState) {
    digitalWrite(RELAY_PIN, STATE);
    Serial.printf("Relay state changed to: %s\n", STATE ? "ON" : "OFF");
  }
  delay(DELAY);
}
