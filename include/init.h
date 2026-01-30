#ifndef INIT_H
#define INIT_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <AiEsp32RotaryEncoder.h>
#include "secrets.h"
#include "button.h"
#include "menu.h"
#include "clock.h"
#include "haiot.h"
#include "StatusBar.h"
#include "icons.h"

// Функції ініціалізації
void setupEncoder();
void setupI2C();
bool setupBME280();
void setupDisplay();
void setupHAIOT();
void setupSensors();
void setupMenu();
void setupStatusBar();

#endif // INIT_H
