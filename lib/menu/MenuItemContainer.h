#ifndef _MENU_ITEM_CONTAINER_H_
#define _MENU_ITEM_CONTAINER_H_

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "MenuItem.h"

class MenuItemContainer : public MenuItem {
public:
    MenuItemContainer(String name, bool enable = true);
    ~MenuItemContainer();
    
    void addChild(MenuItem* child);
    MenuItem* getChild(size_t index) override;
    size_t getChildCount() override;
    
protected:
    void calculateSize() override;
    void render(Adafruit_SSD1306& display, int x, int y, bool selected) override;
    void onClick() override;
    
private:
    size_t childCount;
    MenuItem** children;
};
#endif
