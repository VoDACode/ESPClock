#include "MenuItemText.h"

MenuItemText::MenuItemText(String text) : MenuItem("", nullptr, true), text(text) {
    calculateSize();
}

MenuItemText::MenuItemText(String text, void (*action)(), bool enable) : MenuItem("", action, enable), text(text) {
    calculateSize();
}

void MenuItemText::setText(String text) {
    this->text = text;
    calculateSize();
}

void MenuItemText::calculateSize() {
    size.width = text.length() * 6;
    size.height = 8;
}

void MenuItemText::render(Adafruit_SSD1306& display, int x, int y, bool selected) {
    display.setCursor(x, y);
    if (selected) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        display.setTextColor(SSD1306_WHITE);
    }
    display.print(text);
    display.setTextColor(SSD1306_WHITE);
}