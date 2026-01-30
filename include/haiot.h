#include <Arduino.h>
#include <EEPROM.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <functional>
#include <vector>

#ifndef HAIOT_H
#define HAIOT_H

// Типи сенсорів Home Assistant
enum class SensorType {
    SENSOR,          // Звичайний сенсор (температура, вологість тощо)
    BINARY_SENSOR,   // Бінарний сенсор (вкл/викл)
    SWITCH,          // Перемикач
    LIGHT,           // Світло
    NUMBER,          // Числове значення
    BUTTON,          // Кнопка
    DATETIME,        // Дата та час
    TIME,            // Тільки час
    DATE             // Тільки дата
};

// Callback функції для обробки команд від HA
using CommandCallback = std::function<void(const String& payload)>;

void EEPROM_WriteString(int &addrOffset, const String &str);
void EEPROM_ReadString(int &addrOffset, String &str);

// Структура для опису сенсора
struct HAIOTSensor {
    String uniqueId;              // Унікальний ID сенсора
    String name;                  // Ім'я сенсора
    SensorType type;              // Тип сенсора
    String deviceClass;           // Клас пристрою для HA (temperature, humidity, motion тощо)
    String unit;                  // Одиниці вимірювання
    String icon;                  // Іконка для HA
    String stateTopic;            // MQTT топік для публікації стану
    String commandTopic;          // MQTT топік для отримання команд (для switch, light тощо)
    String configTopic;           // MQTT топік для Discovery конфігурації
    CommandCallback onCommand;    // Callback для обробки команд
    String currentState;          // Поточний стан
    float minValue = NAN;         // Мінімальне значення для NUMBER
    float maxValue = NAN;         // Максимальне значення для NUMBER
    float step = NAN;             // Крок для NUMBER
    
    HAIOTSensor(const String& id, const String& nm, SensorType t)
        : uniqueId(id), name(nm), type(t) {}
};

struct MQTTInfo
{
    String server = "";
    uint16_t port = 1883;
    String user = "";
    String password = "";
    String clientID = "";

    void saveToEEPROM(int &addrOffset);
    void readFromEEPROM(int &addrOffset);
};


struct WiFiInfo
{
    String ssid = "";
    String password = "";

    void saveToEEPROM(int &addrOffset);
    void readFromEEPROM(int &addrOffset);
};

struct HAIOTInfo
{
    MQTTInfo mqtt;
    WiFiInfo wifi;
    String deviceName = "HAIOT_Device";
    String location = "Home";
    void saveToEEPROM(int startAddr);
    void readFromEEPROM(int startAddr);
};

class HAIOT {
private:
    HAIOTInfo info;
    int eepromStartAddr = 0;
    bool debugMode = false;
    WiFiClient espClient;
    PubSubClient mqttClient{espClient};
    std::vector<HAIOTSensor*> sensors;
    bool discoveryPublished = false;
    unsigned long lastStatePublish = 0;
    unsigned long statePublishInterval = 60000; // Публікувати стан кожні 60 секунд
    unsigned long wifiReconnectAttempt = 0;
    unsigned long wifiReconnectInterval = 5000; // Спроба підключення кожні 5 секунд
    bool wifiConnecting = false;
    bool mqttConnecting = false; // Флаг процесу підключення до MQTT

    void wifiDeamon();
    void mqttDeamon();
    void log(const String &message);
    void onMQTTMessage(char* topic, byte* payload, unsigned int length);
    void publishDiscovery();
    void publishSensorState(HAIOTSensor* sensor);
    String buildConfigPayload(HAIOTSensor* sensor);
    String getComponentName(SensorType type);

public:
    HAIOT(int eepromAddr = 0);
    void setDebugMode(bool mode);
    HAIOTInfo& getInfo();
    void start();
    void tick();
    
    // API для реєстрації сенсорів
    HAIOTSensor* registerSensor(const String& uniqueId, const String& name, SensorType type);
    HAIOTSensor* registerSensor(const String& uniqueId, const String& name, SensorType type, const String& deviceClass);
    HAIOTSensor* registerSensor(const String& uniqueId, const String& name, SensorType type, const String& deviceClass, const String& unit);
    
    // Оновлення стану сенсора
    void setSensorState(const String& uniqueId, const String& state);
    void setSensorState(HAIOTSensor* sensor, const String& state);
    
    // Встановлення callback для команд
    void setCommandCallback(const String& uniqueId, CommandCallback callback);
    void setCommandCallback(HAIOTSensor* sensor, CommandCallback callback);
    
    // Знайти сенсор за ID
    HAIOTSensor* findSensor(const String& uniqueId);
    
    // Встановити інтервал публікації стану (мс)
    void setStatePublishInterval(unsigned long interval);
    
    // Встановити іконку для сенсора
    void setSensorIcon(HAIOTSensor* sensor, const String& icon);
    void setSensorIcon(const String& uniqueId, const String& icon);
    
    // Встановити діапазон для NUMBER сенсора
    void setSensorRange(HAIOTSensor* sensor, float min, float max);
    void setSensorRange(const String& uniqueId, float min, float max);
    
    // Встановити крок для NUMBER сенсора
    void setSensorStep(HAIOTSensor* sensor, float step);
    void setSensorStep(const String& uniqueId, float step);
    
    // WiFi status
    bool isWiFiConnected();
    
    // Налаштування WiFi
    void setWiFiReconnectInterval(unsigned long interval);
};

#endif
