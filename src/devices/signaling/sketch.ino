#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi настройки
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

// Пины устройств
#define SMOKE_PIN 12     // Пин для датчика дыма
#define FLAME_PIN 13     // Пин для датчика пламени
#define ONE_WIRE_BUS 4   // Пин для датчика температуры DS18B20
#define LED_PIN 22       // Пин для индикаторной лампочки
#define BUZZER_PIN 15    // Пин для пищалки
#define HEALTH_PIN 0     // Пин для индикатора здоровья

// Настройки сервера
#define SERVER "http://109.194.163.31:8751"
#define DEVICE_NAME "FIRE-ALARM-1"
#define TOKEN "fire_alarm_token"

// Константы
#define DELAY 2000       // Задержка между отправками (2 секунды)
#define BUZZER_FREQ 1000 // Частота пищалки
#define ALARM_INTERVAL 500 // Интервал прерывистого сигнала
#define TEMP_THRESHOLD 60.0 // Порог температуры для срабатывания сигнализации (градусов Цельсия)

// Инициализация OneWire и DallasTemperature
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Переменные для датчиков
int smokeValue;
int flameValue;
float temperatureValue;
bool alarmActive = false;
bool lastSmokeState = false;
bool lastFlameState = false;
bool lastTempState = false;

// Переменные для управления пищалкой
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

// Адрес устройства DS18B20 (если нужно работать с конкретным датчиком)
DeviceAddress tempDeviceAddress;

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
    digitalWrite(HEALTH_PIN, HIGH);
  } else {
    Serial.println(" Failed to connect!");
    digitalWrite(HEALTH_PIN, LOW);
  }
  
  Serial.println();
  delay(1000);
}

bool getSmokeStatus() {
  return smokeValue == 0;
}

bool getFlameStatus() {
  return flameValue == 0;
}

bool getTemperatureStatus() {
  return temperatureValue >= TEMP_THRESHOLD;
}

bool isAlarmActive() {
  return getSmokeStatus() || getFlameStatus() || getTemperatureStatus();
}

StaticJsonDocument<200> requestDocument() {
  StaticJsonDocument<200> doc;
  
  JsonObject values = doc.createNestedObject("values");
  values["smoke"] = int(getSmokeStatus());
  values["flame"] = int(getFlameStatus());
  values["temperature"] = temperatureValue;
  values["alarm"] = int(isAlarmActive());

  doc["token"] = TOKEN;
  
  return doc;
}

void sendState() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, will try later");
    digitalWrite(HEALTH_PIN, LOW);
    return;
  }

  HTTPClient http;
  
  String url = String(SERVER) + "/api/v1/heartbeat/" DEVICE_NAME;
  
  Serial.println(url);
  http.begin(url);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");

  String requestBody;
  serializeJson(requestDocument(), requestBody);
  
  Serial.print("Request body: ");
  Serial.println(requestBody);

  int httpCode = http.POST(requestBody);
  
  if (httpCode != 200) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    http.end();
    digitalWrite(HEALTH_PIN, LOW);
    return;
  }

  String payload = http.getString();
  Serial.print("Response: ");
  Serial.println(payload);
  
  http.end();
  digitalWrite(HEALTH_PIN, HIGH);
}

void readSensors() {
  smokeValue = digitalRead(SMOKE_PIN);
  flameValue = digitalRead(FLAME_PIN);
  
  // Запрашиваем температуру у всех датчиков на шине
  sensors.requestTemperatures();
  
  // Читаем температуру с первого датчика
  temperatureValue = sensors.getTempCByIndex(0);
  
  // Проверяем, что чтение прошло успешно
  if (temperatureValue == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Could not read temperature data");
    temperatureValue = 0.0;
  }
}

void printStatus() {
  Serial.print("Smoke: ");
  if (smokeValue) {
    Serial.print("-");
  } else {
    Serial.print("Detected!");
  }
  
  Serial.print("  |  Flame: ");
  if (flameValue) {
    Serial.print("-");
  } else {
    Serial.print("Detected!");
  }
  
  Serial.print("  |  Temperature: ");
  Serial.print(temperatureValue);
  Serial.print("°C");
  
  if (getTemperatureStatus()) {
    Serial.print(" (High!)");
  }
  
  if (isAlarmActive()) {
    Serial.println("  |  ALARM! ALARM! ALARM!");
  } else {
    Serial.println("  |  All clear");
  }
}

void handleAlarm() {
  bool alarm = isAlarmActive();
  
  if (alarm) {
    if (!alarmActive) {
      alarmActive = true;
      Serial.println("ALARM ACTIVATED!");
    }
    
    unsigned long currentMillis = millis();
    if (currentMillis - lastBuzzerToggle >= ALARM_INTERVAL) {
      buzzerState = !buzzerState;
      if (buzzerState) {
        tone(BUZZER_PIN, BUZZER_FREQ);
      } else {
        noTone(BUZZER_PIN);
      }
      lastBuzzerToggle = currentMillis;
    }
  } else {
    if (alarmActive) {
      alarmActive = false;
      noTone(BUZZER_PIN);
      Serial.println("Alarm deactivated");
    }
  }
}

void setup() {
  Serial.begin(115200); 
  
  // Настройка пинов
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SMOKE_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(HEALTH_PIN, OUTPUT);
  
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("Fire Alarm System initializing...");
  
  // Инициализация датчиков температуры
  sensors.begin();
  
  // Проверяем, найден ли датчик температуры
  if (!sensors.getAddress(tempDeviceAddress, 0)) {
    Serial.println("Unable to find address for Device 0");
  } else {
    Serial.print("Found DS18B20 with address: ");
    for (uint8_t i = 0; i < 8; i++) {
      Serial.print(tempDeviceAddress[i], HEX);
      if (i < 7) Serial.print(":");
    }
    Serial.println();
    
    // Устанавливаем разрешение датчика (9-12 бит)
    sensors.setResolution(tempDeviceAddress, 12);
    
    // Читаем установленное разрешение
    Serial.print("Resolution set to: ");
    Serial.print(sensors.getResolution(tempDeviceAddress));
    Serial.println(" bits");
  }
  
  delay(200);
  
  connectToWiFi();
  
  Serial.println("System ready!");
}

void loop() {
  readSensors();

  printStatus();

  handleAlarm();

  sendState();

  delay(DELAY);
}