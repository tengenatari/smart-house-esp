#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define SOIL_PIN 34
#define RELAY_PIN 26
#define BUTTON_PIN 18
#define LED_PIN 21
#define HEALTH_PIN 2
#define FLOW_SENSOR_PIN 5  

#define I2C_SDA 23
#define I2C_SCL 22

#define SERVER "http://109.194.163.31:8751"
#define DEVICE_NAME "PLANT-WATERING-1"
#define TOKEN "848f2b4e-eeb8-4037-936c-f1e92ef88b0c"

#define SOIL_ADC_WET 2165
#define SOIL_ADC_DRY 3135

#define SOIL_MOISTURE_DRY_THRESHOLD 35
#define SOIL_MOISTURE_WET_THRESHOLD 55

#define MANUAL_WATERING_TIME 5000
#define AUTO_WATERING_MAX_TIME 10000
#define FORCED_WATERING_TIME 15000

#define SEND_INTERVAL 2000
#define STATUS_INTERVAL 1000
#define LCD_INTERVAL 500
#define SOIL_READ_INTERVAL 500

#define AUTO_DRY_COOLDOWN 60000UL        
#define FORCE_WATERING_INTERVAL 420000UL  


volatile int flowPulseCount = 0;  
float flowRate = 0.0f;           
unsigned long lastFlowTime = 0;   
unsigned long lastFlowRateTime = 0;  

int soilRawValue = 0;
int soilMoisturePercent = 0;
unsigned long lastWateringTime = 0;
unsigned long lastAutoDryWateringTime = 0;

bool pumpState = false;
bool manualModeWatering = false;
bool autoModeWatering = false;
bool forcedModeWatering = false;

unsigned long manualWateringStart = 0;
unsigned long autoWateringStart = 0;
unsigned long forcedWateringStart = 0;

int suspiciousDryAutoCount = 0;
int sensorWarning = 0;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long lastSendTime = 0;
unsigned long lastStatusTime = 0;
unsigned long lastLcdTime = 0;
unsigned long lastSoilReadTime = 0;

void connectToWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
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
}

void setPump(bool state) {
  pumpState = state;

  if (state) {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
  }
}

void readSoilIfNeeded() {
  if (millis() - lastSoilReadTime < SOIL_READ_INTERVAL) {
    return;
  }

  lastSoilReadTime = millis();

  soilRawValue = analogRead(SOIL_PIN);
  soilMoisturePercent = map(soilRawValue, SOIL_ADC_DRY, SOIL_ADC_WET, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);
}

void simulateFlow() {
  if (pumpState) {
    flowRate += random(0, 15);  
    if (flowRate > 100) flowRate = 100;  

  } else {
    flowRate = 0;
  }
}

void updateLCD() {
  if (millis() - lastLcdTime < LCD_INTERVAL) {
    return;
  }

  lastLcdTime = millis();

  lcd.setCursor(0, 0);
  lcd.print("Soil:");
  lcd.print(getSoilStateText());
  lcd.print(" ");
  lcd.print((int)soilMoisturePercent);
  lcd.print("%");

  lcd.setCursor(0, 1);
  if (manualModeWatering) {
    lcd.print("MANUAL     ");
  } else if (forcedModeWatering) {
    lcd.print("FORCED       ");
  } else if (autoModeWatering) {
    lcd.print("AUTO          ");
  } else {
    lcd.print("IDLE       ");
  }
}

void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {
        Serial.println("Manual watering button pressed");

        manualModeWatering = true;
        autoModeWatering = false;
        forcedModeWatering = false;

        manualWateringStart = millis();
        lastWateringTime = millis();

        setPump(true);
      }
    }
  }

  lastButtonReading = reading;
}

void handleManualWatering() {
  if (manualModeWatering && millis() - manualWateringStart >= MANUAL_WATERING_TIME) {
    manualModeWatering = false;

    if (!autoModeWatering && !forcedModeWatering) {
      setPump(false);
    }

    Serial.println("Manual watering finished");
  }
}

void handleAutoWatering() {
  if (manualModeWatering || forcedModeWatering) return;

  if (!autoModeWatering && soilMoisturePercent <= SOIL_MOISTURE_DRY_THRESHOLD) {
    if (millis() - lastAutoDryWateringTime >= AUTO_DRY_COOLDOWN) {
      autoModeWatering = true;
      autoWateringStart = millis();

      lastWateringTime = millis();
      lastAutoDryWateringTime = millis();

      setPump(true);
      Serial.println("Auto watering started: soil too dry");
    }
  }

  if (autoModeWatering) {
    if (soilMoisturePercent >= SOIL_MOISTURE_WET_THRESHOLD) {
      autoModeWatering = false;
      setPump(false);

      suspiciousDryAutoCount = 0;
      sensorWarning = 0;

      Serial.println("Auto watering stopped: soil moisture restored");
      return;
    }

    if (millis() - autoWateringStart >= AUTO_WATERING_MAX_TIME) {
      autoModeWatering = false;
      setPump(false);

      if (soilMoisturePercent <= SOIL_MOISTURE_DRY_THRESHOLD) {
        suspiciousDryAutoCount++;
        if (suspiciousDryAutoCount >= 3) {
          sensorWarning = 1;
          suspiciousDryAutoCount = 0;
        }
      } else {
        suspiciousDryAutoCount = 0;
        sensorWarning = 0;
      }

      Serial.println("Auto watering stopped: max time reached");
      return;
    }
  }
}

void handleForcedWatering() {
  if (manualModeWatering || autoModeWatering || forcedModeWatering) return;

  if (millis() - lastWateringTime >= FORCE_WATERING_INTERVAL) {
    forcedModeWatering = true;
    forcedWateringStart = millis();
    lastWateringTime = millis();

    setPump(true);
    Serial.println("Forced watering started: long time without watering");
  }

  if (forcedModeWatering && millis() - forcedWateringStart >= FORCED_WATERING_TIME) {
    forcedModeWatering = false;
    setPump(false);
    Serial.println("Forced watering finished");
  }
}

void IRAM_ATTR flowPulse() {
  flowPulseCount++;
}

String getSoilStateText() {
  if (soilRawValue < SOIL_ADC_WET) return "WET";
  if (soilRawValue > SOIL_ADC_DRY) return "DRY";
  return "OK";
}

StaticJsonDocument<512> buildRequest() {
  StaticJsonDocument<512> doc;

  int mode = manualModeWatering ? 0 : 1;

  JsonObject values = doc.createNestedObject("values");
  values["soil_raw"] = soilMoisturePercent;
  values["pump"] = pumpState ? 1 : 0;
  values["mode"] = mode;
  values["last_watering_ms"] = millis() - lastWateringTime;
  values["sensor_warning"] = sensorWarning;
  values["dry_auto_count"] = suspiciousDryAutoCount;
  values["flow_rate"] = flowRate; 

  doc["token"] = TOKEN;

  return doc;
}

void sendState() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    digitalWrite(HEALTH_PIN, LOW);
    return;
  }

  HTTPClient http;
  String url = String(SERVER) + "/api/v1/heartbeat/" DEVICE_NAME;

  http.begin(url);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");

  String body;
  StaticJsonDocument<512> doc = buildRequest();
  serializeJson(doc, body);

  Serial.print("POST ");
  Serial.println(url);
  Serial.print("Request body: ");
  Serial.println(body);

  int httpCode = http.POST(body);

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.printf("HTTP code: %d\n", httpCode);
    Serial.print("Response: ");
    Serial.println(payload);
    digitalWrite(HEALTH_PIN, HIGH);
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
    digitalWrite(HEALTH_PIN, LOW);
  }

  http.end();
}


void printStatus() {
  Serial.print("Soil raw: ");
  Serial.print(soilRawValue);

  Serial.print(" | Moisture: ");
  Serial.print(soilMoisturePercent);
  Serial.print("%");

  Serial.print(" | SoilState: ");
  Serial.print(getSoilStateText());

  Serial.print(" | Pump: ");
  Serial.print(pumpState ? "ON" : "OFF");

  Serial.print(" | Mode: ");
  if (manualModeWatering) {
    Serial.print("MANUAL");
  } else if (forcedModeWatering) {
    Serial.print("FORCED_AUTO");
  } else if (autoModeWatering) {
    Serial.print("AUTO");
  } else {
    Serial.print("AUTO_IDLE");
  }

  Serial.print(" | Flow Rate: ");
  Serial.print(flowRate);
  Serial.println(" L/min");
}

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.begin(115200);
  randomSeed(micros());

  lcd.init();
  lcd.backlight();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(HEALTH_PIN, OUTPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulse, FALLING); 

  setPump(false);
  digitalWrite(HEALTH_PIN, LOW);

  delay(200);
  soilRawValue = analogRead(SOIL_PIN);
  soilMoisturePercent = map(soilRawValue, SOIL_ADC_DRY, SOIL_ADC_WET, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  lastWateringTime = millis();
  lastAutoDryWateringTime = millis() - AUTO_DRY_COOLDOWN;

  Serial.println("Plant watering system with analog soil sensor and water flow sensor initializing...");

  connectToWiFi();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System ready");

  Serial.println("System ready!");
}

void loop() {
  readSoilIfNeeded();
  handleButton();
  handleManualWatering();
  handleAutoWatering();
  handleForcedWatering();
  updateLCD();

  simulateFlow();

  if (millis() - lastStatusTime >= STATUS_INTERVAL) {
    lastStatusTime = millis();
    printStatus();
  }

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();
    sendState();
  }

  delay(20);
}