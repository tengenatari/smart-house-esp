#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define HEALTH_PIN 0
#define BUZZER_PIN 4
#define BUTTON_PIN 15
#define SERVER "http://iversy.ru:8751"
#define DEVICE_NAME "WATCH-1"
#define TOKEN "abacaba"
#define DELAY 500

#define DAY 86400
#define HOUR 3600
#define MIN 60

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     5*HOUR
#define UTC_OFFSET_DST 0

LiquidCrystal_I2C LCD(0x27, 16, 2);

boolean SUCCESS = false;

int TIME = 0;
int ALARM_LENGTH = 5*MIN; 

int CURRENT_COUNTER = 0;
int UPDATE_ALARMS_COUNTER = 100;
int ALARMS[100] = {};

int alarmCount = 0;
bool alarmActive = false;
int activeAlarmIndex = -1;
int alarmStartTime = 0; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET, 60000);


bool isButtonPressed() {
  return !boolean(digitalRead(BUTTON_PIN));
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(DELAY);
    Serial.print(".");
    attempts++;
  }
  
  timeClient.begin();
}

StaticJsonDocument<200> requestDocument() {
  StaticJsonDocument<200> values;
  StaticJsonDocument<200> doc;

  values["state"] = 0;
  doc["values"] = values;
  doc["token"] = TOKEN;

  return doc;
}

int timeToSec(const char* timeStr) {
  char buffer[3];
  
  buffer[0] = timeStr[0];
  buffer[1] = timeStr[1];
  buffer[2] = '\0';
  int hours = atoi(buffer);
  
  buffer[0] = timeStr[3];
  buffer[1] = timeStr[4];
  int minutes = atoi(buffer);
  
  buffer[0] = timeStr[6];
  buffer[1] = timeStr[7];
  int seconds = atoi(buffer);
  
  return hours*HOUR + minutes*MIN + seconds;
}

void processResponse(HTTPClient &http, String payload) {
  StaticJsonDocument<200> responseDoc;
  DeserializationError error = deserializeJson(responseDoc, payload);
      
  if (error) {
    http.end();
    SUCCESS = false;
    return;
  }
  
  JsonArray alarms = responseDoc["metadata"]["alarms"].as<JsonArray>();
  alarmCount = 0;
  
  for (JsonObject alarm : alarms) {
    if (alarmCount >= 100) break;
    ALARMS[alarmCount] = timeToSec(alarm["time"]);
    alarmCount++;
  }
  
  SUCCESS = true;
  http.end();
}

void showLC() {
  LCD.setCursor(0, 0);
  if (alarmActive)
    LCD.print("    ALARM!!!");
  else
    LCD.print("            ");
  LCD.setCursor(0, 1);
  LCD.print("    ");
  LCD.print(timeClient.getFormattedTime());
  LCD.print("    ");
}

void updateTime() {
  timeClient.update();
  TIME = timeClient.getHours()*HOUR +
    timeClient.getMinutes()*MIN +
    timeClient.getSeconds();
}

void alarmLoop() {
  alarmActive = true;
  for (int i=0; i<ALARM_LENGTH; i++){
    if (isButtonPressed())
      break;
    digitalWrite(BUZZER_PIN, LOW);
    updateTime();
    showLC();
    delay(1000);
    digitalWrite(BUZZER_PIN, HIGH);
  }
  alarmActive = false;
  digitalWrite(BUZZER_PIN, HIGH);
}

void checkAlarms() {
  for (int i = 0; i < alarmCount; i++) {
    if (checkAlarm(ALARMS[i])) {
      alarmLoop();
      ALARMS[i] = (ALARMS[i]-ALARM_LENGTH+DAY) % DAY;
      break;
    }
  }
}

void updateAlarms() {
  SUCCESS = false;

  HTTPClient http;
  
  String url = String(SERVER) + "/api/v1/heartbeat/"+ DEVICE_NAME;
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

boolean checkAlarm(int alarm) {
  int start = alarm % DAY;
  
  if (alarm + ALARM_LENGTH < DAY) {
    return (TIME >= start) && (TIME <= (start + ALARM_LENGTH));
  }
  return (TIME >= start) || (TIME <= (start + ALARM_LENGTH) % DAY);
}

void setup(void) {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(HEALTH_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, HIGH);
  
  LCD.init();
  LCD.backlight();
  
  connectToWiFi();
}

void loop(void) {
  updateTime();
  showLC();
  checkAlarms();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Reconnecting...");
    digitalWrite(HEALTH_PIN, LOW);
    connectToWiFi();
    delay(DELAY);
    return;
  }

  if (CURRENT_COUNTER%UPDATE_ALARMS_COUNTER == 0) {
    updateAlarms();
  }
  CURRENT_COUNTER = (CURRENT_COUNTER+1)%UPDATE_ALARMS_COUNTER;
  
  delay(DELAY);
}