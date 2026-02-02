#ifndef _MENU_ITEM_TOGGLE_H_
#define _MENU_ITEM_TOGGLE_H_

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "MenuItem.h"

class MenuItemToggle : public MenuItem {
public:
    MenuItemToggle(String text, bool* valuePtr, bool enable = true);
    void setText(String text);
    bool getValue();
    void setValue(bool value);
    void toggle();
    
protected:
    void calculateSize() override;
    void render(Adafruit_SSD1306& display, int x, int y, bool selected) override;
    void onClick() override;
    
private:
    String text;
    bool* valuePtr;
};

#endif
