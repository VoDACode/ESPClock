#include "MenuItemToggle.h"

MenuItemToggle::MenuItemToggle(String text, bool* valuePtr, bool enable)
    : MenuItem("", nullptr, enable), text(text), valuePtr(valuePtr) {
    calculateSize();
}

void MenuItemToggle::setText(String text) {
    this->text = text;
    calculateSize();
}

bool MenuItemToggle::getValue() {
    return valuePtr != nullptr ? *valuePtr : false;
}

void MenuItemToggle::setValue(bool value) {
    if (valuePtr != nullptr) {
        *valuePtr = value;
    }
}

void MenuItemToggle::toggle() {
    if (valuePtr != nullptr) {
        *valuePtr = !(*valuePtr);
    }
}

void MenuItemToggle::calculateSize() {
    // Текст + "[X]" або "[ ]"
    int textWidth = text.length() * 6;
    int checkboxWidth = 3 * 6; // "[X]" = 3 символи
    size.width = textWidth + 6 + checkboxWidth; // 6 пікселів відступ
    size.height = 8;
}

void MenuItemToggle::render(Adafruit_SSD1306& display, int x, int y, bool selected) {
    display.setCursor(x, y);
    
    if (selected) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        display.setTextColor(SSD1306_WHITE);
    }
    
    display.print(text);
    display.print(" ");
    
    bool value = getValue();
    display.print(value ? "[X]" : "[ ]");
    
    display.setTextColor(SSD1306_WHITE);
}

void MenuItemToggle::onClick() {
    toggle();
    MenuItem::onClick();
}
