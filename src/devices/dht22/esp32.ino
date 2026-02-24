#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define HEALTH_PIN 0
#define DHT_PIN 4
#define SERVER "http://109.194.163.31:8751"
#define DEVICE_NAME "DHT22-1"
#define TOKEN "abacaba"
#define DELAY 5000
#define DHTTYPE DHT22

LiquidCrystal_I2C LCD(0x27, 16, 2);
DHT DHT(DHT_PIN, DHTTYPE);

boolean SUCCESS = false;

float TEMPERATURE = 0.0;
float HUMIDITY = 0.0;

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
  values["temperature"] = TEMPERATURE;
  values["humidity"] = HUMIDITY;

  StaticJsonDocument<100> doc;
  doc["token"] = TOKEN;
  doc["values"] = values;
  return doc;
}

void processResponce(HTTPClient &http, String payload) {
  SUCCESS = true;
  http.end();
}

void sendState() {
  SUCCESS = false;  
  HTTPClient http;
  
  String url = String(SERVER) + "/api/v1/heartbeat/" DEVICE_NAME;

  Serial.println(url);
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
      
  processResponce(http, payload);
}

void readSensor() {
  TEMPERATURE = DHT.readTemperature();
  HUMIDITY = DHT.readHumidity();
}

void printSensor(char* name, char* unit, float val) {
  LCD.print(name);
  LCD.print(": ");
  LCD.print(val);
  LCD.print(" ");
  LCD.print(unit);
}

void showLC() {
  LCD.setCursor(0, 0);
  printSensor("TEM", "C", TEMPERATURE);
  LCD.setCursor(0, 1);
  printSensor("HUM", "%", HUMIDITY);
}

void setup(void) {
  Serial.begin(115200);
  
  pinMode(HEALTH_PIN, OUTPUT);
  pinMode(DHT_PIN, INPUT);
  
  DHT.begin();
  LCD.begin(16,2);
  LCD.init();
  LCD.backlight();
}

void loop(void) {
  readSensor();
  showLC();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Reconnecting...");
    digitalWrite(HEALTH_PIN, LOW);
    connectToWiFi();
    delay(DELAY);
    return;
  }

  sendState();
  
  digitalWrite(HEALTH_PIN, SUCCESS);
  if (!SUCCESS) {
    delay(DELAY);
    return;
  }

  delay(DELAY);
}
