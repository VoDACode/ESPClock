#ifndef _MENU_ITEM_TEXT_H_
#define _MENU_ITEM_TEXT_H_

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "MenuItem.h"

class MenuItemText : public MenuItem {
public:
    MenuItemText(String text);
    MenuItemText(String text, void (*action)(), bool enable = true);
    void setText(String text);
protected:
    void calculateSize() override;
    void render(Adafruit_SSD1306& display, int x, int y, bool selected) override;
private:
    String text;
};

#endif