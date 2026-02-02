#include <Arduino.h>
#include <TaskTimer.h>
#include <haiot.h>
#include <clock.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include <AiEsp32RotaryEncoder.h>
#include "button.h"
#include "menu.h"
#include "init.h"
#include "StatusBar.h"

HAIOT haiot;
TaskTimer taskTimer;

// Локальний годинник - працює незалежно від WiFi
unsigned long localMillis = 0;
int localHours = 0;
int localMinutes = 0;
int localSeconds = 0;
int localDay = 1;
int localMonth = 1;
int localYear = 2026;
bool timeInitialized = false; // Чи був час синхронізований з NTP

// WiFi Task removed - now handled in main loop

// BME280 сенсор
Adafruit_BME280 bme;
bool bmeConnected = false;

// Енкодер EC11
#define ENCODER_CLK 2
#define ENCODER_DT 1
#define ENCODER_SW 0
#define ROTARY_ENCODER_STEPS 4

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW, -1, ROTARY_ENCODER_STEPS);
Button encoderButton(ENCODER_SW, true, 50); // Кнопка енкодера з debounce 50ms
int lastEncoderPos = 0;
unsigned long lastEncoderTime = 0;

// Сенсори
HAIOTSensor *currentTimeSensor;
HAIOTSensor *currentDateSensor;
HAIOTSensor *displayBrightness;
HAIOTSensor *timezoneOffset;
HAIOTSensor *temperatureSensor;
HAIOTSensor *pressureSensor;
HAIOTSensor *altitudeSensor;
HAIOTSensor *humiditySensor;
HAIOTSensor *wifiStatusSensor;
HAIOTSensor *wifiRssiSensor;
HAIOTSensor *wifiIpSensor;

Menu mainMenu;
bool inMenu = false;

StatusBar statusBar(display);
StatusBar::Element* syncStatus;
StatusBar::Element* wifiStatus;
StatusBar::Element* alarmStatus;

// Змінні для будильника
int brightnessLevel = 255;
int timezoneOffsetHours = 2;

// Callback для зміни енкодера
void IRAM_ATTR readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

// Оновлення локального часу
void updateLocalTime()
{
  unsigned long currentMillis = millis();
  unsigned long elapsed = currentMillis - localMillis;

  if (elapsed >= 1000) // Кожну секунду
  {
    localMillis = currentMillis;
    localSeconds++;

    if (localSeconds >= 60)
    {
      localSeconds = 0;
      localMinutes++;

      if (localMinutes >= 60)
      {
        localMinutes = 0;
        localHours++;

        if (localHours >= 24)
        {
          localHours = 0;
          localDay++;

          // Проста логіка днів в місяці
          int daysInMonth = 31;
          if (localMonth == 2)
            daysInMonth = 28;
          else if (localMonth == 4 || localMonth == 6 || localMonth == 9 || localMonth == 11)
            daysInMonth = 30;

          if (localDay > daysInMonth)
          {
            localDay = 1;
            localMonth++;

            if (localMonth > 12)
            {
              localMonth = 1;
              localYear++;
            }
          }
        }
      }
    }
  }
}

// Синхронізація з NTP
void syncLocalTimeWithNTP()
{
  if (haiot.isWiFiConnected())
  {
    timeClient.update();
    localHours = timeClient.getHours();
    localMinutes = timeClient.getMinutes();
    localSeconds = timeClient.getSeconds();

    time_t rawTime = timeClient.getEpochTime();
    struct tm *ti = localtime(&rawTime);
    localDay = ti->tm_mday;
    localMonth = ti->tm_mon + 1;
    localYear = ti->tm_year + 1900;

    localMillis = millis();
    timeInitialized = true;

    Serial.println("Time synced with NTP: " + String(localHours) + ":" + String(localMinutes) + ":" + String(localSeconds));
  }
}

void readBME280()
{
  if (!bmeConnected)
    return;

  float temp = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F; // Конвертувати в hPa
  float altitude = bme.readAltitude(1013.25);   // Висота відносно рівня моря
  float humidity = bme.readHumidity();          // Вологість повітря

  haiot.setSensorState(temperatureSensor, String(temp, 1));
  haiot.setSensorState(pressureSensor, String(pressure, 1));
  haiot.setSensorState(altitudeSensor, String(altitude, 0));
  haiot.setSensorState(humiditySensor, String(humidity, 1));
}

void setup()
{
  Serial.begin(115200);
  delay(5000); // Дати час на ініціалізацію Serial

  // Ініціалізація всіх компонентів
  setupEncoder();
  setupI2C();
  setupBME280();
  setupDisplay();
  setupHAIOT();
  setupSensors();

  // NTP Client - запускається в WiFi task
  timeClient.begin();
  timeClient.setTimeOffset(timezoneOffsetHours * 3600);

  // Ініціалізуємо локальний час початковими значеннями
  localMillis = millis();
  Serial.println("Local clock started. Will sync with NTP when WiFi connects.");

  // Задачі
  if (bmeConnected)
  {
    taskTimer.add(readBME280, 5000, true); // Читати BME280 кожні 5 секунд
  }

  display.clearDisplay();
  display.display();

  setupMenu();
  setupStatusBar();

  Serial.println("\n=== Setup Complete ===");
  Serial.println("Encoder: CLK=GPIO2, DT=GPIO1, SW=GPIO0");
  if (bmeConnected)
  {
    Serial.println("BME280: Connected on I2C (SDA=GPIO8, SCL=GPIO10)");
  }
}

void loop()
{
  // Оновлення стану кнопки енкодера
  encoderButton.update();

  // Оновлення локального часу (незалежно від WiFi)
  updateLocalTime();

  // Обробка WiFi та MQTT в головному циклі (з невеликою затримкою для stability)
  static unsigned long lastHaiotTick = 0;
  if (millis() - lastHaiotTick > 250) // Викликати tick не частіше ніж раз на 250мс
  {
    haiot.tick();
    lastHaiotTick = millis();
  }

  // Синхронізація часу кожні 1с, якщо WiFi підключений
  static unsigned long lastNtpSync = 0;
  if (haiot.isWiFiConnected() && (millis() - lastNtpSync > 1000))
  {
    syncLocalTimeWithNTP();
    lastNtpSync = millis();
  }

  // Обробка задач (малювання годинника)
  taskTimer.tick();

  if (inMenu)
  {
    // === Обробка енкодера ===
    if (rotaryEncoder.encoderChanged())
    {
      int encoderPos = rotaryEncoder.readEncoder();
      int diff = encoderPos - lastEncoderPos;
      lastEncoderPos = encoderPos;

      Serial.println("Encoder moved: " + String(diff));

      if (diff > 0)
      {
        mainMenu.action(MenuAction(MENU_ACTION_ROLL, -1));
      }
      else if (diff < 0)
      {
        mainMenu.action(MenuAction(MENU_ACTION_ROLL, 1));
      }
    }

    // Обробка кнопки енкодера
    // Перевіряємо подвійний клік - повернення назад
    if (encoderButton.wasClickedNTimes(2, 500))
    {
      Serial.println("Double click detected - navigating to parent");
      mainMenu.action(MenuAction(MENU_ACTION_BACK));
    }
    // Одинарний клік - виконання дії
    else if (encoderButton.wasClickedNTimes(1, 500))
    {
      Serial.println("Single click detected - executing action");
      mainMenu.action(MenuAction(MENU_ACTION_CLICK));
    }
    // Якщо більше 2 кліків - просто скидаємо (ігноруємо)
    else
    {
      encoderButton.wasClickedNTimes(3, 500); // Спробуємо 3+ - це очистить лічильник
    }
  }

  // === Оновлення сенсорів кожні 2 секунди ===
  static unsigned long lastSensorUpdate = 0;
  if (millis() - lastSensorUpdate > 2000)
  {
    if (haiot.isWiFiConnected())
    {
      // Оновити поточний час і дату (використовуємо локальний)
      char timeStr[9];
      sprintf(timeStr, "%02d:%02d:%02d", localHours, localMinutes, localSeconds);
      haiot.setSensorState(currentTimeSensor, String(timeStr));

      char dateStr[11];
      sprintf(dateStr, "%04d-%02d-%02d", localYear, localMonth, localDay);
      haiot.setSensorState(currentDateSensor, String(dateStr));

      // Оновити статус WiFi
      String wifiStatusStr = "Connected";
      if (timeInitialized)
        wifiStatusStr += " (Time Synced)";
      haiot.setSensorState(wifiStatusSensor, wifiStatusStr);

      // Оновити RSSI
      haiot.setSensorState(wifiRssiSensor, String(WiFi.RSSI()));

      // Оновити IP address
      haiot.setSensorState(wifiIpSensor, WiFi.localIP().toString());
    }
    else
    {
      // WiFi відсутній
      haiot.setSensorState(wifiStatusSensor, "Disconnected - Reconnecting...");
      haiot.setSensorState(wifiRssiSensor, "0");
      haiot.setSensorState(wifiIpSensor, "0.0.0.0");
    }

    lastSensorUpdate = millis();
  }

  if (inMenu)
  {
    mainMenu.displayMenu(display);
  }
  else
  {
    display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_BLACK);
    syncStatus->visible = haiot.isWiFiConnected() && timeInitialized;
    wifiStatus->visible = haiot.isWiFiConnected();
    drawClock();
    statusBar.draw();
    display.display();
    if(encoderButton.isLongPressed(1000))
    {
      Serial.println("Entering menu...");
      inMenu = true;
      mainMenu.displayMenu(display);
    }
  }
}