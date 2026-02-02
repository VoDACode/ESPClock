#ifndef _MENU_ITEM_H_
#define _MENU_ITEM_H_

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "MenuAction.h"

struct MenuItemSize {
    size_t width;
    size_t height;
};

class MenuItem {
friend class Menu;
public: 
    bool enable;
    MenuItem* parent;
    virtual size_t getChildCount();
    virtual MenuItem* getChild(size_t index);
    String getName();

    MenuItem(String name = "");
    MenuItem(String name, void (*action)(), bool enable = true);
    virtual ~MenuItem();

    MenuItemSize getSize();
protected:
    void (*action)();
    MenuItemSize size;
    String name;
    void virtual calculateSize();
    void virtual render(Adafruit_SSD1306& display, int x, int y, bool selected) = 0;
    void virtual onClick();
    bool virtual onAction(MenuAction action);
};

#endif
