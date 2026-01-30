#include <Arduino.h>
#include <NTPClient.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <WiFiUdp.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

extern Adafruit_SSD1306 display;

extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

// Зовнішні змінні (визначені в main.cpp)
extern int localHours;
extern int localMinutes;
extern int localSeconds;
extern int localDay;
extern int localMonth;
extern int localYear;
extern bool timeInitialized;

void drawClock();