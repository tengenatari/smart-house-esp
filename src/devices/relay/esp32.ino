#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define HEALTH_PIN 0
#define SERVER "http://109.194.163.31:8751/"

boolean helthcheck() {
  Serial.print("Sending HTTP request to ");
  Serial.print(SERVER);

  HTTPClient http;
  
  http.begin(SERVER);
  http.setTimeout(10000);

  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("\nResponse:");
      Serial.println("-------------------");
      Serial.println(payload);
      Serial.println("-------------------");
    } else {
      Serial.printf("Error: Unexpected HTTP code: %d\n", httpCode);
    }
  } else {
    Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return (httpCode == 200);
}

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

void setup(void) {
  Serial.begin(115200);
  connectToWiFi();
  pinMode(HEALTH_PIN, OUTPUT);
}

void loop(void) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Reconnecting...");
    connectToWiFi();
    return;
  }

  if (helthcheck()) {
    digitalWrite(HEALTH_PIN, HIGH);
  } else {
    digitalWrite(HEALTH_PIN, LOW);
  }
  delay(5000);
}
