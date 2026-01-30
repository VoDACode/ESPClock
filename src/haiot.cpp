#include "haiot.h"

void EEPROM_WriteString(int &addrOffset, const String &str)
{
    int len = str.length();
    EEPROM.put(addrOffset, len);
    addrOffset += sizeof(len);
    for (int i = 0; i < len; i++)
    {
        EEPROM.put(addrOffset++, str[i]);
    }
}

void EEPROM_ReadString(int &addrOffset, String &str)
{
    int len = 0;
    EEPROM.get(addrOffset, len);
    addrOffset += sizeof(len);
    str = "";
    for (int i = 0; i < len; i++)
    {
        char c;
        EEPROM.get(addrOffset++, c);
        str += c;
    }
}

void MQTTInfo::saveToEEPROM(int &addrOffset)
{
    EEPROM_WriteString(addrOffset, server);
    EEPROM.put(addrOffset, port);
    addrOffset += sizeof(port);
    EEPROM_WriteString(addrOffset, user);
    EEPROM_WriteString(addrOffset, password);
    EEPROM_WriteString(addrOffset, clientID);
}

void MQTTInfo::readFromEEPROM(int &addrOffset)
{
    EEPROM_ReadString(addrOffset, server);
    EEPROM.get(addrOffset, port);
    addrOffset += sizeof(port);
    EEPROM_ReadString(addrOffset, user);
    EEPROM_ReadString(addrOffset, password);
    EEPROM_ReadString(addrOffset, clientID);
}

void HAIOTInfo::saveToEEPROM(int startAddr)
{
    int addrOffset = startAddr;
    mqtt.saveToEEPROM(addrOffset);
    EEPROM_WriteString(addrOffset, deviceName);
    EEPROM_WriteString(addrOffset, location);
}

void HAIOTInfo::readFromEEPROM(int startAddr)
{
    int addrOffset = startAddr;
    mqtt.readFromEEPROM(addrOffset);
    EEPROM_ReadString(addrOffset, deviceName);
    EEPROM_ReadString(addrOffset, location);
}

HAIOT::HAIOT(int eepromAddr)
    : eepromStartAddr(eepromAddr)
{
}

void HAIOT::setDebugMode(bool mode)
{
    this->debugMode = mode;
}

void HAIOT::log(const String &message)
{
    if (debugMode)
    {
        Serial.println("[HAIOT] " + message);    
    }
}

HAIOTInfo& HAIOT::getInfo()
{
    return info;
}
    
void HAIOT::start()
{
#ifdef ESP32
    EEPROM.begin(512);
#else
    EEPROM.begin(this->eepromStartAddr);
#endif
    
    log("HAIOT started. Device Name: " + info.deviceName);
    
    // Ініціалізація WiFi - обов'язково!
    if (!info.wifi.ssid.isEmpty())
    {
        log("Initializing WiFi in Station mode...");
        
        // Спочатку повністю вимикаємо WiFi
        WiFi.mode(WIFI_OFF);
        delay(100);
        
        // Встановлюємо режим станції
        WiFi.mode(WIFI_STA);
        delay(100);
        
        // Налаштування WiFi для стабільного підключення
        WiFi.persistent(false); // Не зберігати WiFi конфігурацію у flash
        WiFi.setAutoConnect(false); // Вимкнути автопідключення
        WiFi.setAutoReconnect(true); // Але дозволити автореконнект після успішного підключення
        
        // КРИТИЧНО: Вимкнути енергозбереження WiFi
        // Це запобігає відключенню ESP32 роутером через відсутність відповіді
        WiFi.setSleep(false);
        log("WiFi power save disabled");
        
        // Встановити hostname для кращої ідентифікації
        WiFi.setHostname(info.deviceName.c_str());
        
        delay(100);
        
        log("Connecting to WiFi: " + info.wifi.ssid);
        WiFi.begin(info.wifi.ssid.c_str(), info.wifi.password.c_str());
        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        
        // Чекаємо підключення (неблокуюче очікування статусу)
        int tryCount = 0;
        while (WiFi.status() != WL_CONNECTED && tryCount < 20)
        {
            delay(500);
            tryCount++;
            
            wl_status_t status = WiFi.status();
            if (tryCount % 4 == 0) // Лог кожні 2 секунди
            {
                String statusStr;
                switch(status) {
                    case WL_IDLE_STATUS: statusStr = "IDLE"; break;
                    case WL_NO_SSID_AVAIL: statusStr = "NO_SSID"; break;
                    case WL_CONNECTED: statusStr = "CONNECTED"; break;
                    case WL_CONNECT_FAILED: statusStr = "FAILED"; break;
                    case WL_DISCONNECTED: statusStr = "DISCONNECTED"; break;
                    default: statusStr = String(status); break;
                }
                log("Connecting... Status: " + statusStr + " (attempt " + String(tryCount) + "/20)");
            }
        }
        
        if (WiFi.status() == WL_CONNECTED)
        {
            log("WiFi connected! IP: " + WiFi.localIP().toString());
            log("Signal: " + String(WiFi.RSSI()) + " dBm");
        }
        else
        {
            log("Failed to connect to WiFi. Will retry in background.");
        }

        // Встановити прапорці для wifiDeamon()
        wifiConnecting = false; // Початкова спроба завершена
        wifiReconnectAttempt = millis(); // Почати відлік для наступної спроби
    }
    else
    {
        log("No WiFi credentials configured");
        WiFi.mode(WIFI_OFF); // Вимкнути WiFi якщо немає credentials
    }

    mqttClient.setServer(info.mqtt.server.c_str(), info.mqtt.port);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->onMQTTMessage(topic, payload, length);
    });
    mqttClient.setBufferSize(1024); // Збільшуємо буфер для Discovery повідомлень
}

void HAIOT::tick()
{
    wifiDeamon();
    mqttDeamon();
    
    // Публікувати Discovery конфігурацію якщо ще не опубліковано
    if (mqttClient.connected() && !discoveryPublished)
    {
        publishDiscovery();
    }
    
    // Публікувати стан сенсорів періодично
    if (millis() - lastStatePublish > statePublishInterval && mqttClient.connected())
    {
        for (auto sensor : sensors)
        {
            if (!sensor->currentState.isEmpty())
            {
                publishSensorState(sensor);
            }
        }
        lastStatePublish = millis();
    }
}

void HAIOT::wifiDeamon()
{
    static unsigned long lastLogTime = 0;
    static unsigned long modeSetTime = 0;
    static bool waitingForModeInit = false;
    
    wl_status_t status = WiFi.status();
    
    if (status != WL_CONNECTED)
    {
        // Логування кожні 5 секунд
        if (millis() - lastLogTime > 5000)
        {
            String statusStr;
            switch(status) {
                case WL_NO_SHIELD: statusStr = "NO_SHIELD"; break;
                case WL_IDLE_STATUS: statusStr = "IDLE"; break;
                case WL_NO_SSID_AVAIL: statusStr = "NO_SSID_AVAILABLE"; break;
                case WL_SCAN_COMPLETED: statusStr = "SCAN_COMPLETED"; break;
                case WL_CONNECTED: statusStr = "CONNECTED"; break;
                case WL_CONNECT_FAILED: statusStr = "CONNECT_FAILED"; break;
                case WL_CONNECTION_LOST: statusStr = "CONNECTION_LOST"; break;
                case WL_DISCONNECTED: statusStr = "DISCONNECTED"; break;
                default: statusStr = "UNKNOWN(" + String(status) + ")"; break;
            }
            log("WiFi disconnected. Status: " + statusStr);
            lastLogTime = millis();
        }
        
        // Неблокуюче перепідключення - двоетапне
        if (!wifiConnecting && !waitingForModeInit && (millis() - wifiReconnectAttempt > wifiReconnectInterval))
        {
            log("Attempting to reconnect to WiFi...");
            WiFi.mode(WIFI_STA);
            modeSetTime = millis();
            waitingForModeInit = true;
        }
        
        // Чекаємо 50мс після встановлення режиму (неблокуюче)
        if (waitingForModeInit && (millis() - modeSetTime > 50))
        {
            log("Re-initializing WiFi connection...");
            log("SSID: " + info.wifi.ssid);
            WiFi.begin(info.wifi.ssid.c_str(), info.wifi.password.c_str());
            wifiConnecting = true;
            wifiReconnectAttempt = millis();
            waitingForModeInit = false;
        }
        
        // Перевірити чи є прогрес у підключенні
        if (wifiConnecting && status == WL_CONNECTED)
        {
            wifiConnecting = false;
        }
    }
    else
    {
        // Підключено
        if (lastLogTime != 0 || wifiConnecting)
        {
            log("Connected to WiFi. IP: " + WiFi.localIP().toString());
            log("Signal strength: " + String(WiFi.RSSI()) + " dBm");
            lastLogTime = 0;
            wifiConnecting = false;
            waitingForModeInit = false;
        }
    }
}

void HAIOT::mqttDeamon()
{
    // Не намагатися підключитися до MQTT без WiFi
    if (WiFi.status() != WL_CONNECTED)
    {
        mqttConnecting = false; // Скинути флаг якщо WiFi відсутній
        return;
    }
    
    if (!mqttClient.connected())
    {
        // Якщо вже в процесі підключення, не повторювати
        if (mqttConnecting)
        {
            return;
        }
        
        // Перевірити чи минув достатній інтервал після останньої спроби
        static unsigned long lastMqttAttempt = 0;
        if (millis() - lastMqttAttempt < 5000) // Спроба кожні 5 секунд
        {
            return;
        }
        lastMqttAttempt = millis();
        mqttConnecting = true; // Встановити флаг підключення
        
        log("Connecting to MQTT broker: " + info.mqtt.server + ":" + String(info.mqtt.port));
        log("Client ID: " + info.mqtt.clientID);
        
        if (mqttClient.connect(info.mqtt.clientID.c_str(), info.mqtt.user.c_str(), info.mqtt.password.c_str()))
        {
            mqttConnecting = false; // Підключення успішне
            log("Connected to MQTT.");
            
            // Підписатися на всі command топіки сенсорів
            for (auto sensor : sensors)
            {
                if (!sensor->commandTopic.isEmpty())
                {
                    mqttClient.subscribe(sensor->commandTopic.c_str());
                    log("Subscribed to: " + sensor->commandTopic);
                }
            }
        }
        else
        {
            int mqttState = mqttClient.state();
            String stateStr;
            switch(mqttState) {
                case -4: stateStr = "TIMEOUT"; break;
                case -3: stateStr = "CONNECTION_LOST"; break;
                case -2: stateStr = "CONNECT_FAILED"; break;
                case -1: stateStr = "DISCONNECTED"; break;
                case 0: stateStr = "CONNECTED"; break;
                case 1: stateStr = "BAD_PROTOCOL"; break;
                case 2: stateStr = "BAD_CLIENT_ID"; break;
                case 3: stateStr = "UNAVAILABLE"; break;
                case 4: stateStr = "BAD_CREDENTIALS"; break;
                case 5: stateStr = "UNAUTHORIZED"; break;
                default: stateStr = "UNKNOWN(" + String(mqttState) + ")"; break;
            }
            log("Failed to connect to MQTT. State: " + stateStr);
            mqttConnecting = false; // Скинути флаг після невдалої спроби
        }
    }
    else
    {
        // Підключено успішно
        mqttConnecting = false; // Переконатися що флаг скинутий
        mqttClient.loop();
    }
}

void HAIOT::onMQTTMessage(char* topic, byte* payload, unsigned int length)
{
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    log("MQTT Message received on topic " + String(topic) + ": " + message);
    
    // Перевірити чи це команда для якогось сенсора
    for (auto sensor : sensors)
    {
        if (sensor->commandTopic == String(topic) && sensor->onCommand)
        {
            log("Executing command callback for sensor: " + sensor->name);
            sensor->onCommand(message);
            break;
        }
    }
}

// API для реєстрації сенсорів

HAIOTSensor* HAIOT::registerSensor(const String& uniqueId, const String& name, SensorType type)
{
    return registerSensor(uniqueId, name, type, "", "");
}

HAIOTSensor* HAIOT::registerSensor(const String& uniqueId, const String& name, SensorType type, const String& deviceClass)
{
    return registerSensor(uniqueId, name, type, deviceClass, "");
}

HAIOTSensor* HAIOT::registerSensor(const String& uniqueId, const String& name, SensorType type, const String& deviceClass, const String& unit)
{
    HAIOTSensor* sensor = new HAIOTSensor(uniqueId, name, type);
    sensor->deviceClass = deviceClass;
    sensor->unit = unit;
    
    // Побудувати топіки
    String baseTopic = "homeassistant/" + getComponentName(type) + "/" + info.deviceName + "/" + uniqueId;
    sensor->configTopic = baseTopic + "/config";
    sensor->stateTopic = baseTopic + "/state";
    
    // Якщо це керований компонент, додати command топік
    if (type == SensorType::SWITCH || type == SensorType::LIGHT || 
        type == SensorType::NUMBER || type == SensorType::BUTTON ||
        type == SensorType::DATETIME || type == SensorType::TIME ||
        type == SensorType::DATE)
    {
        sensor->commandTopic = baseTopic + "/set";
    }
    
    sensors.push_back(sensor);
    log("Registered sensor: " + name + " (" + uniqueId + ")");
    
    // Якщо вже підключені до MQTT, опублікувати Discovery
    if (mqttClient.connected() && discoveryPublished)
    {
        publishDiscovery();
    }
    
    return sensor;
}

// Оновлення стану сенсора

void HAIOT::setSensorState(const String& uniqueId, const String& state)
{
    HAIOTSensor* sensor = findSensor(uniqueId);
    if (sensor)
    {
        setSensorState(sensor, state);
    }
}

void HAIOT::setSensorState(HAIOTSensor* sensor, const String& state)
{
    if (sensor)
    {
        sensor->currentState = state;
        if (mqttClient.connected())
        {
            publishSensorState(sensor);
        }
    }
}

// Встановлення callback для команд

void HAIOT::setCommandCallback(const String& uniqueId, CommandCallback callback)
{
    HAIOTSensor* sensor = findSensor(uniqueId);
    if (sensor)
    {
        setCommandCallback(sensor, callback);
    }
}

void HAIOT::setCommandCallback(HAIOTSensor* sensor, CommandCallback callback)
{
    if (sensor)
    {
        sensor->onCommand = callback;
    }
}

// Знайти сенсор за ID

HAIOTSensor* HAIOT::findSensor(const String& uniqueId)
{
    for (auto sensor : sensors)
    {
        if (sensor->uniqueId == uniqueId)
        {
            return sensor;
        }
    }
    return nullptr;
}

void HAIOT::setStatePublishInterval(unsigned long interval)
{
    this->statePublishInterval = interval;
}

void HAIOT::setSensorIcon(HAIOTSensor* sensor, const String& icon)
{
    if (sensor)
    {
        sensor->icon = icon;
    }
}

void HAIOT::setSensorIcon(const String& uniqueId, const String& icon)
{
    HAIOTSensor* sensor = findSensor(uniqueId);
    setSensorIcon(sensor, icon);
}

void HAIOT::setSensorRange(HAIOTSensor* sensor, float min, float max)
{
    if (sensor)
    {
        sensor->minValue = min;
        sensor->maxValue = max;
    }
}

void HAIOT::setSensorRange(const String& uniqueId, float min, float max)
{
    HAIOTSensor* sensor = findSensor(uniqueId);
    setSensorRange(sensor, min, max);
}

void HAIOT::setSensorStep(HAIOTSensor* sensor, float step)
{
    if (sensor)
    {
        sensor->step = step;
    }
}

void HAIOT::setSensorStep(const String& uniqueId, float step)
{
    HAIOTSensor* sensor = findSensor(uniqueId);
    setSensorStep(sensor, step);
}

bool HAIOT::isWiFiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

void HAIOT::setWiFiReconnectInterval(unsigned long interval)
{
    this->wifiReconnectInterval = interval;
    log("WiFi reconnect interval set to: " + String(interval) + " ms");
}

// Допоміжні методи для публікації

void HAIOT::publishDiscovery()
{
    log("Publishing MQTT Discovery configurations...");
    
    // Публікуємо по одному сенсору за виклик для неблокуючої роботи
    static size_t currentSensorIndex = 0;
    static unsigned long lastPublishTime = 0;
    
    if (sensors.size() == 0)
        return;
    
    // Публікуємо один сенсор за раз, кожні 100мс
    if (millis() - lastPublishTime > 100)
    {
        if (currentSensorIndex < sensors.size())
        {
            HAIOTSensor* sensor = sensors[currentSensorIndex];
            String payload = buildConfigPayload(sensor);
            bool published = mqttClient.publish(sensor->configTopic.c_str(), payload.c_str(), true);
            
            if (published)
            {
                log("Published discovery for: " + sensor->name);
            }
            else
            {
                log("Failed to publish discovery for: " + sensor->name);
                log("MQTT State: " + String(mqttClient.state()));
            }
            
            currentSensorIndex++;
            lastPublishTime = millis();
            
            // Перевірити чи це був останній сенсор
            if (currentSensorIndex >= sensors.size())
            {
                log("All sensors published to MQTT Discovery");
                discoveryPublished = true;
                currentSensorIndex = 0;
            }
        }
    }
}

void HAIOT::publishSensorState(HAIOTSensor* sensor)
{
    if (mqttClient.publish(sensor->stateTopic.c_str(), sensor->currentState.c_str()))
    {
        log("Published state for " + sensor->name + ": " + sensor->currentState);
    }
}

String HAIOT::buildConfigPayload(HAIOTSensor* sensor)
{
    String payload = "{";
    payload += "\"name\":\"" + sensor->name + "\",";
    payload += "\"unique_id\":\"" + sensor->uniqueId + "\",";
    payload += "\"state_topic\":\"" + sensor->stateTopic + "\"";
    
    if (!sensor->commandTopic.isEmpty())
    {
        payload += ",\"command_topic\":\"" + sensor->commandTopic + "\"";
    }
    
    if (!sensor->deviceClass.isEmpty())
    {
        payload += ",\"device_class\":\"" + sensor->deviceClass + "\"";
    }
    
    if (!sensor->unit.isEmpty())
    {
        payload += ",\"unit_of_measurement\":\"" + sensor->unit + "\"";
    }
    
    if (!sensor->icon.isEmpty())
    {
        payload += ",\"icon\":\"" + sensor->icon + "\"";
    }
    
    // Додати min/max/step для NUMBER сенсорів
    if (sensor->type == SensorType::NUMBER)
    {
        if (!isnan(sensor->minValue))
        {
            payload += ",\"min\":" + String(sensor->minValue, 2);
        }
        if (!isnan(sensor->maxValue))
        {
            payload += ",\"max\":" + String(sensor->maxValue, 2);
        }
        if (!isnan(sensor->step))
        {
            payload += ",\"step\":" + String(sensor->step, 2);
        }
    }
    
    // Додати інформацію про пристрій
    payload += ",\"device\":{";
    payload += "\"identifiers\":[\"" + info.deviceName + "\"],";
    payload += "\"name\":\"" + info.deviceName + "\",";
    payload += "\"manufacturer\":\"HAIOT\",";
    payload += "\"model\":\"ESP8266\"";
    if (!info.location.isEmpty())
    {
        payload += ",\"suggested_area\":\"" + info.location + "\"";
    }
    payload += "}";
    
    payload += "}";
    
    return payload;
}

String HAIOT::getComponentName(SensorType type)
{
    switch (type)
    {
        case SensorType::SENSOR: return "sensor";
        case SensorType::BINARY_SENSOR: return "binary_sensor";
        case SensorType::SWITCH: return "switch";
        case SensorType::LIGHT: return "light";
        case SensorType::NUMBER: return "number";
        case SensorType::BUTTON: return "button";
        case SensorType::DATETIME: return "datetime";
        case SensorType::TIME: return "time";
        case SensorType::DATE: return "date";
        default: return "sensor";
    }
}