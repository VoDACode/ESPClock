#include "init.h"

// Зовнішні змінні з main.cpp
extern AiEsp32RotaryEncoder rotaryEncoder;
extern Button encoderButton;
extern int lastEncoderPos;
extern Adafruit_BME280 bme;
extern bool bmeConnected;
extern HAIOT haiot;
extern HAIOTSensor *currentTimeSensor;
extern HAIOTSensor *currentDateSensor;
extern HAIOTSensor *displayBrightness;
extern HAIOTSensor *timezoneOffset;
extern HAIOTSensor *temperatureSensor;
extern HAIOTSensor *pressureSensor;
extern HAIOTSensor *altitudeSensor;
extern HAIOTSensor *humiditySensor;
extern HAIOTSensor *wifiStatusSensor;
extern HAIOTSensor *wifiRssiSensor;
extern HAIOTSensor *wifiIpSensor;
extern Menu mainMenu;
extern bool inMenu;
extern StatusBar statusBar;
extern StatusBar::Element* syncStatus;
extern StatusBar::Element* wifiStatus;
extern StatusBar::Element* alarmStatus;
extern int brightnessLevel;
extern int timezoneOffsetHours;

// Зовнішні функції
extern void IRAM_ATTR readEncoderISR();

void setupEncoder()
{
    Serial.println("Initializing Encoder...");
    
    // Ініціалізація енкодера EC11 з бібліотекою AiEsp32RotaryEncoder
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(0, 1000, false); // minValue, maxValue, circleValues
    rotaryEncoder.setAcceleration(250);          // Прискорення при швидкому обертанні
    rotaryEncoder.disableAcceleration();         // Вимкнути прискорення для більш точного контролю

    // Встановлюємо початкове значення на середину діапазону
    rotaryEncoder.setEncoderValue(500);
    lastEncoderPos = rotaryEncoder.readEncoder();

    // Ініціалізація кнопки енкодера
    encoderButton.begin();
    
    Serial.println("Encoder and button initialized!");
    Serial.print("Encoder initial position: ");
    Serial.println(lastEncoderPos);
}

void setupI2C()
{
    Serial.println("Initializing I2C...");
    
    // ESP32-C3: GPIO8 (SDA), GPIO10 (SCL)
    Wire.begin(8, 10);
    delay(100); // Дати час на стабілізацію I2C шини

    Serial.println("I2C Scanner:");
    // Простий сканер I2C
    for (uint8_t address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Serial.print(" - Found I2C device at 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");
        }
    }
}

bool setupBME280()
{
    Serial.println("Initializing BME280...");

    // Прочитати chip ID напряму
    Wire.beginTransmission(0x76);
    Wire.write(0xD0); // Регістр chip ID
    Wire.endTransmission();
    Wire.requestFrom(0x76, 1);
    if (Wire.available())
    {
        uint8_t chipID = Wire.read();
        Serial.print("Chip ID at 0x76: 0x");
        Serial.println(chipID, HEX);
        Serial.println("Expected: 0x58 (BMP280), 0x60 (BME280), 0x55 (BMP180)");
    }

    // BME280 вже ініціалізований з &Wire
    unsigned status = bme.begin(0x76);
    Serial.print("BME280 begin() status: 0x");
    Serial.println(status, HEX);

    if (status)
    {
        bmeConnected = true;
        Serial.println("BME280 initialized successfully!");

        // Налаштування BME280
        bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                        Adafruit_BME280::SAMPLING_X2,  // Температура
                        Adafruit_BME280::SAMPLING_X16, // Тиск
                        Adafruit_BME280::SAMPLING_X16, // Вологість
                        Adafruit_BME280::FILTER_X16,
                        Adafruit_BME280::STANDBY_MS_500);
        return true;
    }
    else
    {
        Serial.println("BME280 not found! Check wiring or try different sensor library.");
        return false;
    }
}

void setupDisplay()
{
    Serial.println("Initializing OLED display...");
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    {
        Serial.println(F("SSD1306 allocation failed"));
        while (1); // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Starting...");
    display.display();
}

void setupHAIOT()
{
    Serial.println("\n=== Configuring HAIOT ===");
    
    haiot.setDebugMode(false);

    HAIOTInfo &info = haiot.getInfo();
    info.wifi.ssid = WIFI_SSID;
    info.wifi.password = WIFI_PASSWORD;
    info.mqtt.server = MQTT_SERVER;
    info.mqtt.port = MQTT_PORT;
    info.mqtt.user = MQTT_USER;
    info.mqtt.password = MQTT_PASSWORD;
    info.mqtt.clientID = MQTT_CLIENT_ID;
    info.deviceName = DEVICE_NAME;
    info.location = LOCATION;
    // Зберегти MQTT конфігурацію в EEPROM
    info.saveToEEPROM(0);

    Serial.println("Starting HAIOT...");
    haiot.setWiFiReconnectInterval(30000);
    haiot.start();
}

void setupSensors()
{
    Serial.println("Registering sensors...");
    
    // 1. Поточний час (читання)
    currentTimeSensor = haiot.registerSensor("clock_time", "Current Time", SensorType::SENSOR);
    haiot.setSensorIcon(currentTimeSensor, "mdi:clock-digital");

    // 2. Поточна дата (читання)
    currentDateSensor = haiot.registerSensor("clock_date", "Current Date", SensorType::SENSOR);
    haiot.setSensorIcon(currentDateSensor, "mdi:calendar");

    // 5. Яскравість дисплею (0-255)
    displayBrightness = haiot.registerSensor("display_brightness", "Display Brightness", SensorType::NUMBER);
    haiot.setSensorIcon(displayBrightness, "mdi:brightness-6");
    haiot.setSensorRange(displayBrightness, 0, 255);
    haiot.setSensorStep(displayBrightness, 5);

    haiot.setCommandCallback(displayBrightness, [](const String &value)
    {
        brightnessLevel = value.toInt();
        brightnessLevel = constrain(brightnessLevel, 0, 255);
        
        // Застосувати яскравість (для SSD1306 використовуємо контраст)
        if (brightnessLevel == 0) {
            display.ssd1306_command(SSD1306_DISPLAYOFF);
        } else {
            display.ssd1306_command(SSD1306_DISPLAYON);
            display.ssd1306_command(SSD1306_SETCONTRAST);
            display.ssd1306_command(brightnessLevel);
        }
        
        Serial.println("Brightness set to: " + String(brightnessLevel));
        haiot.setSensorState(displayBrightness, String(brightnessLevel));
    });

    // 6. Зміщення часового поясу
    timezoneOffset = haiot.registerSensor("timezone_offset", "Timezone Offset", SensorType::NUMBER);
    haiot.setSensorIcon(timezoneOffset, "mdi:clock-time-eight");
    haiot.setSensorRange(timezoneOffset, -12, 14);
    haiot.setSensorStep(timezoneOffset, 1);

    haiot.setCommandCallback(timezoneOffset, [](const String &value)
    {
        timezoneOffsetHours = value.toInt();
        timezoneOffsetHours = constrain(timezoneOffsetHours, -12, 14);
        timeClient.setTimeOffset(timezoneOffsetHours * 3600);
        Serial.println("Timezone offset set to: UTC+" + String(timezoneOffsetHours));
        haiot.setSensorState(timezoneOffset, String(timezoneOffsetHours));
    });

    // 7. Температура з BME280
    if (bmeConnected)
    {
        temperatureSensor = haiot.registerSensor("bme280_temperature", "Temperature", 
                                                  SensorType::SENSOR, "temperature", "°C");
        haiot.setSensorIcon(temperatureSensor, "mdi:thermometer");

        // 8. Атмосферний тиск
        pressureSensor = haiot.registerSensor("bme280_pressure", "Atmospheric Pressure", 
                                               SensorType::SENSOR, "pressure", "hPa");
        haiot.setSensorIcon(pressureSensor, "mdi:gauge");

        // 9. Висота
        altitudeSensor = haiot.registerSensor("bme280_altitude", "Altitude", 
                                               SensorType::SENSOR, "", "m");
        haiot.setSensorIcon(altitudeSensor, "mdi:altimeter");

        // 10. Вологість повітря
        humiditySensor = haiot.registerSensor("bme280_humidity", "Humidity", 
                                               SensorType::SENSOR, "humidity", "%");
        haiot.setSensorIcon(humiditySensor, "mdi:water-percent");
    }

    // 11. Статус WiFi
    wifiStatusSensor = haiot.registerSensor("wifi_status", "WiFi Status", SensorType::SENSOR);
    haiot.setSensorIcon(wifiStatusSensor, "mdi:wifi");

    // 12. WiFi Signal Strength (RSSI)
    wifiRssiSensor = haiot.registerSensor("wifi_rssi", "WiFi Signal Strength", 
                                          SensorType::SENSOR, "signal_strength", "dBm");
    haiot.setSensorIcon(wifiRssiSensor, "mdi:wifi-strength-2");

    // 13. WiFi IP Address
    wifiIpSensor = haiot.registerSensor("wifi_ip", "WiFi IP Address", SensorType::SENSOR);
    haiot.setSensorIcon(wifiIpSensor, "mdi:ip-network");

    // Встановити початкові значення
    haiot.setSensorState(displayBrightness, String(brightnessLevel));
    haiot.setSensorState(timezoneOffset, String(timezoneOffsetHours));
}

void setupMenu()
{
    Serial.println("Setting up menu...");
    
    MenuItem *rootItem = new MenuItem("Main Menu");
    rootItem->addChild(new MenuItem("Set Alarm Time", []()
    { 
        Serial.println("Set Alarm Time selected"); 
    }));
    
    rootItem->addChild(new MenuItem("Toggle Alarm", []()
    { 
        Serial.println("Toggle Alarm selected"); 
    }));

    MenuItem *settingsItem = new MenuItem("Settings");
    settingsItem->addChild(new MenuItem("Display Brightness", []()
    { 
        Serial.println("Display Brightness selected"); 
    }));
    
    settingsItem->addChild(new MenuItem("Timezone Offset", []()
    { 
        Serial.println("Timezone Offset selected"); 
    }));
    
    rootItem->addChild(settingsItem);
    rootItem->addChild(new MenuItem("Exit Menu", []()
    { 
        Serial.println("Exit Menu selected"); 
        inMenu = false; 
    }));

    mainMenu.setRootItem(rootItem);
}

void setupStatusBar()
{
    Serial.println("Setting up status bar...");
    
    syncStatus = new StatusBar::Element("Sync");
    syncStatus->visible = false;
    syncStatus->side = StatusBar::Element::STATUS_BAR_ELEMENT_LEFT;

    wifiStatus = new StatusBar::Element(ICON_WIFI_CONNECTED_bits, ICON_WIFI_CONNECTED_width, ICON_WIFI_CONNECTED_height);
    wifiStatus->side = StatusBar::Element::STATUS_BAR_ELEMENT_RIGHT;
    wifiStatus->visible = false;

    alarmStatus = new StatusBar::Element("Alarm");
    alarmStatus->side = StatusBar::Element::STATUS_BAR_ELEMENT_RIGHT;
    alarmStatus->visible = false;

    statusBar.addElement(syncStatus);
    statusBar.addElement(wifiStatus);
    statusBar.addElement(alarmStatus);
}