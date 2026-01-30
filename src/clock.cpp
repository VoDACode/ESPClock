#include "clock.h"
#include <WiFi.h>

// Визначення глобальних об'єктів
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void drawClock()
{
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    // Відображаємо локальний час (працює завжди)
    display.setCursor((SCREEN_WIDTH - 12 * 8) / 2, (SCREEN_HEIGHT - 16) / 2);
    display.setTextSize(2);
    display.printf("%02d:%02d:%02d", localHours, localMinutes, localSeconds);

    // Відображаємо дату
    display.setTextSize(1);
    display.setCursor((SCREEN_WIDTH - 10 * 6) / 2, SCREEN_HEIGHT - 10);
    display.printf("%02d-%02d-%04d", localDay, localMonth, localYear);

    // Показуємо статус синхронізації
    display.setCursor(0, 0);
    display.setTextSize(1);
}
