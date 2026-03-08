#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Stepper.h>

// WiFi настройки
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

// Пины
#define HEALTH_PIN 2        // Встроенный светодиод
#define BTN_OPEN 13         // Зелёная кнопка (открыть)
#define BTN_CLOSE 12        // Красная кнопка (закрыть)
#define MODE_SWITCH 15      // Переключатель режимов (ручной/автоматический)

// Пины для концевиков
#define ENDSTOP_CLOSED 16   // Концевик полностью закрыто
#define ENDSTOP_OPEN 17     // Концевик полностью открыто

// Пины для шагового двигателя
#define IN1 14  // A+
#define IN2 27  // A-
#define IN3 26  // B+
#define IN4 25  // B-

// Настройки сервера
#define SERVER "http://109.194.163.31:8751"
#define DEVICE_NAME "CURTAINS-1"
#define TOKEN "curtains_token"
#define DELAY 20

// Настройки двигателя
#define STEPS_PER_REV 200        // Шагов на оборот
#define DEFAULT_FULL_RANGE 1600  // Шагов на полный ход (по умолчанию)
#define MOTOR_SPEED 30           // Скорость двигателя (об/мин)

// Временные настройки
#define SEND_DELAY 2000          // Отправлять через 2 секунды после остановки
#define SERVER_CHECK_INTERVAL 10000 // Проверка сервера раз в 10 секунд
#define ENDSTOP_DEBOUNCE 50       // Антидребезг концевиков (мс)
#define MOTOR_STEP_DELAY 20       // Задержка между шагами двигателя (мс)

Stepper motor(STEPS_PER_REV, IN1, IN2, IN3, IN4);

// Переменные состояния
int currentPosition = 0;           // Текущая позиция в процентах (0-100)
int targetPosition = 0;            // Целевая позиция в процентах
int lastSentPosition = -1;         // Последняя отправленная позиция
int lastSentMode = -1;             // Последний отправленный режим
int actualFullRange = DEFAULT_FULL_RANGE; // Реальный диапазон после калибровки
int currentStepPosition = 0;       // Текущая позиция в шагах от закрытого положения

// Режимы работы
bool manualMode = true;
bool lastModeSwitchState = HIGH;
bool motorRunning = false;

// Для отслеживания кнопок
unsigned long lastMotorStopTime = 0;
unsigned long lastServerCheck = 0;
unsigned long lastMotorStepTime = 0;

// Для антидребезга концевиков
bool lastClosedEndstopState = HIGH;
bool lastOpenEndstopState = HIGH;
unsigned long lastClosedEndstopTime = 0;
unsigned long lastOpenEndstopTime = 0;
bool closedEndstopActive = false;
bool openEndstopActive = false;

// Калибровочные значения
bool calibrated = false;
bool calibrationMode = false;

// Функция безопасного движения с проверкой концевиков
bool safeStep(int direction) {
    // Проверяем текущее состояние концевиков
    bool closedEndstop = digitalRead(ENDSTOP_CLOSED) == LOW;
    bool openEndstop = digitalRead(ENDSTOP_OPEN) == LOW;
    
    if (direction < 0 && closedEndstop) {
        // Уже в закрытом положении
        currentStepPosition = 0;
        currentPosition = 0;
        if (targetPosition > 0) targetPosition = 0;
        motorRunning = false;
        return false;
    }
    
    if (direction > 0 && openEndstop) {
        // Уже в открытом положении
        currentStepPosition = actualFullRange;
        currentPosition = 100;
        if (targetPosition < 100) targetPosition = 100;
        motorRunning = false;
        return false;
    }
    
    // Выполняем один шаг
    motor.step(direction);
    currentStepPosition += direction;
    
    // Обновляем позицию в процентах на основе реальных шагов
    if (actualFullRange > 0) {
        currentPosition = constrain(map(currentStepPosition, 0, actualFullRange, 0, 100), 0, 100);
    }
    
    // Проверяем, не достигли ли концевика после шага
    delay(5);
    closedEndstop = digitalRead(ENDSTOP_CLOSED) == LOW;
    openEndstop = digitalRead(ENDSTOP_OPEN) == LOW;
    
    if (closedEndstop) {
        currentStepPosition = 0;
        currentPosition = 0;
        if (targetPosition > 0) targetPosition = 0;
    } else if (openEndstop) {
        currentStepPosition = actualFullRange;
        currentPosition = 100;
        if (targetPosition < 100) targetPosition = 100;
    }
    
    return true;
}

// Калибровка начальной позиции
void calibrate() {
    calibrationMode = true;
    
    Serial.println("Starting calibration...");
    motor.setSpeed(MOTOR_SPEED / 2);
    
    // Ищем закрытое положение
    int timeout = 0;
    while (digitalRead(ENDSTOP_CLOSED) != LOW && timeout < 5000) {
        motor.step(-1);
        delay(5);
        timeout++;
    }
    
    if (timeout >= 5000) {
        Serial.println("ERROR: Closed endstop not found!");
        calibrationMode = false;
        return;
    }
    
    Serial.println("Closed endstop detected");
    delay(500);
    
    currentStepPosition = 0;
    currentPosition = 0;
    targetPosition = 0;
    
    // Ищем открытое положение и считаем шаги
    int stepCount = 0;
    timeout = 0;
    
    while (digitalRead(ENDSTOP_OPEN) != LOW && timeout < 10000) {
        motor.step(1);
        stepCount++;
        delay(5);
        timeout++;
    }
    
    if (timeout >= 10000) {
        Serial.println("ERROR: Open endstop not found!");
        calibrationMode = false;
        return;
    }
    
    Serial.println("Open endstop detected");
    
    actualFullRange = stepCount;
    currentStepPosition = stepCount;
    currentPosition = 100;
    targetPosition = 100;
    
    calibrated = true;
    calibrationMode = false;
    motor.setSpeed(MOTOR_SPEED);
    
    Serial.print("Calibration complete! Range: ");
    Serial.print(actualFullRange);
    Serial.println(" steps");
}

// Функция установки целевой позиции
void setTargetPosition(int percent) {
    percent = constrain(percent, 0, 100);
    
    if (!calibrated || calibrationMode) {
        return;
    }
    
    bool closedEndstop = digitalRead(ENDSTOP_CLOSED) == LOW;
    bool openEndstop = digitalRead(ENDSTOP_OPEN) == LOW;
    
    if (percent == 0 && closedEndstop) {
        currentPosition = 0;
        currentStepPosition = 0;
        targetPosition = 0;
        return;
    }
    
    if (percent == 100 && openEndstop) {
        currentPosition = 100;
        currentStepPosition = actualFullRange;
        targetPosition = 100;
        return;
    }
    
    targetPosition = percent;
}

// Функции для отправки на сервер
StaticJsonDocument<200> createRequestDoc() {
    StaticJsonDocument<200> values;
    values["position"] = currentPosition;
    values["mode"] = manualMode ? "manual" : "auto";

    StaticJsonDocument<200> doc;
    doc["token"] = TOKEN;
    doc["values"] = values;
    return doc;
}

void processResponse(HTTPClient &http, String payload) {
    Serial.print("Server response: ");
    Serial.println(payload);
    
    StaticJsonDocument<300> responseDoc;
    DeserializationError error = deserializeJson(responseDoc, payload);
      
    if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return;
    }

    if (responseDoc.containsKey("trigger_active") && responseDoc.containsKey("state")) {
        bool triggerActive = responseDoc["trigger_active"];
        float serverState = responseDoc["state"];
        
        Serial.print("Trigger: ");
        Serial.print(triggerActive ? "ACTIVE" : "INACTIVE");
        Serial.print(", State: ");
        Serial.println(serverState);
        
        if (!manualMode && triggerActive && !calibrationMode) {
            int targetFromServer = constrain(int(serverState), 0, 100);
            Serial.print("Setting target from server: ");
            Serial.print(targetFromServer);
            Serial.println("%");
            setTargetPosition(targetFromServer);
        }
    }
    
    if (responseDoc.containsKey("message")) {
        const char* message = responseDoc["message"];
        Serial.print("Server message: ");
        Serial.println(message);
    }
}

void sendState() {
    if (currentPosition == lastSentPosition && manualMode == lastSentMode) {
        return;
    }
    
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

    String requestBody;
    serializeJson(createRequestDoc(), requestBody);
    
    Serial.print("Sending to server: ");
    Serial.println(requestBody);

    int httpCode = http.POST(requestBody);
    
    Serial.print("HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == 200) {
        String payload = http.getString();
        processResponse(http, payload);
        lastSentPosition = currentPosition;
        lastSentMode = manualMode;
        digitalWrite(HEALTH_PIN, HIGH);
    } else {
        Serial.println("Failed to send to server");
        digitalWrite(HEALTH_PIN, LOW);
    }
    
    http.end();
}

// Проверка переключателя режимов
void checkModeSwitch() {
    bool currentSwitchState = digitalRead(MODE_SWITCH);
    
    if (currentSwitchState != lastModeSwitchState) {
        delay(50);
        currentSwitchState = digitalRead(MODE_SWITCH);
        
        if (currentSwitchState != lastModeSwitchState) {
            manualMode = (currentSwitchState == HIGH);
            
            Serial.print("Mode changed to: ");
            Serial.println(manualMode ? "MANUAL" : "AUTO");
            
            if (!calibrationMode) {
                targetPosition = currentPosition;
                motorRunning = false;
            }
            
            lastModeSwitchState = currentSwitchState;
            lastSentPosition = -1;
            lastSentMode = -1;
            sendState();
        }
    }
}

// Проверка концевиков
void checkEndstops() {
    bool currentClosed = digitalRead(ENDSTOP_CLOSED) == LOW;
    bool currentOpen = digitalRead(ENDSTOP_OPEN) == LOW;
    unsigned long now = millis();
    
    if (currentClosed != lastClosedEndstopState) {
        lastClosedEndstopTime = now;
    }
    
    if (now - lastClosedEndstopTime > ENDSTOP_DEBOUNCE) {
        if (currentClosed != closedEndstopActive) {
            closedEndstopActive = currentClosed;
            if (currentClosed) {
                Serial.println("Closed endstop triggered");
                currentStepPosition = 0;
                currentPosition = 0;
                if (targetPosition > 0 && !calibrationMode) {
                    targetPosition = 0;
                }
                motorRunning = false;
            }
        }
        lastClosedEndstopState = currentClosed;
    }
    
    if (currentOpen != lastOpenEndstopState) {
        lastOpenEndstopTime = now;
    }
    
    if (now - lastOpenEndstopTime > ENDSTOP_DEBOUNCE) {
        if (currentOpen != openEndstopActive) {
            openEndstopActive = currentOpen;
            if (currentOpen) {
                Serial.println("Open endstop triggered");
                currentStepPosition = actualFullRange;
                currentPosition = 100;
                if (targetPosition < 100 && !calibrationMode) {
                    targetPosition = 100;
                }
                motorRunning = false;
            }
        }
        lastOpenEndstopState = currentOpen;
    }
}

// Обработка кнопок в ручном режиме
void handleManualMode() {
    if (calibrationMode) return;
    
    bool openBtn = digitalRead(BTN_OPEN) == LOW;
    bool closeBtn = digitalRead(BTN_CLOSE) == LOW;
    
    if (openBtn && closeBtn) {
        motorRunning = false;
        return;
    }
    
    if (openBtn) {
        if (!openEndstopActive && currentPosition < 100) {
            safeStep(1);
            delay(20);
        }
    }
    
    if (closeBtn) {
        if (!closedEndstopActive && currentPosition > 0) {
            safeStep(-1);
            delay(20);
        }
    }
}

// Движение к целевой позиции (для автоматического режима)
void moveToTarget() {
    if (!calibrated || calibrationMode) return;
    
    unsigned long now = millis();
    
    if (currentPosition == targetPosition) {
        if (motorRunning) {
            motorRunning = false;
            lastMotorStopTime = now;
        }
        return;
    }
    
    if (now - lastMotorStepTime < MOTOR_STEP_DELAY) {
        return;
    }
    
    if (currentPosition < targetPosition) {
        if (!openEndstopActive) {
            if (safeStep(1)) {
                motorRunning = true;
                lastMotorStepTime = now;
            }
        } else {
            targetPosition = currentPosition;
            motorRunning = false;
        }
    } else {
        if (!closedEndstopActive) {
            if (safeStep(-1)) {
                motorRunning = true;
                lastMotorStepTime = now;
            }
        } else {
            targetPosition = currentPosition;
            motorRunning = false;
        }
    }
}

// Проверка команд с сервера
void checkServerCommands() {
    unsigned long now = millis();
    
    if (now - lastServerCheck > SERVER_CHECK_INTERVAL) {
        lastServerCheck = now;
        
        if (!manualMode && !calibrationMode) {
            // В автоматическом режиме - проверяем команды каждые 10 секунд
            Serial.println("Checking server for commands...");
            lastSentPosition = -1;
            sendState();
        } else {
            // В ручном режиме - отправляем состояние раз в 10 секунд, если были изменения
            if (currentPosition != lastSentPosition || manualMode != lastSentMode) {
                Serial.println("Sending state to server");
                sendState();
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== Smart Curtains Starting ===");
    
    pinMode(HEALTH_PIN, OUTPUT);
    pinMode(BTN_OPEN, INPUT_PULLUP);
    pinMode(BTN_CLOSE, INPUT_PULLUP);
    pinMode(MODE_SWITCH, INPUT_PULLUP);
    pinMode(ENDSTOP_CLOSED, INPUT_PULLUP);
    pinMode(ENDSTOP_OPEN, INPUT_PULLUP);
    
    motor.setSpeed(MOTOR_SPEED);
    
    lastModeSwitchState = digitalRead(MODE_SWITCH);
    manualMode = (lastModeSwitchState == HIGH);
    
    calibrate();
    
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        digitalWrite(HEALTH_PIN, HIGH);
    } else {
        Serial.println(" Failed!");
        digitalWrite(HEALTH_PIN, LOW);
    }
    
    sendState();
    
    Serial.println("System ready");
    Serial.print("Mode: ");
    Serial.println(manualMode ? "MANUAL" : "AUTO");
}

void loop() {
    checkModeSwitch();
    checkEndstops();
    
    if (manualMode) {
        handleManualMode();
    } else {
        moveToTarget();
    }
    
    checkServerCommands();
    
    delay(DELAY);
}