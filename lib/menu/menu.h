#ifndef _MENU_H_
#define _MENU_H_

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "MenuAction.h"
#include "MenuItem.h"
#include "MenuItemText.h"
#include "MenuItemToggle.h"
#include "MenuItemContainer.h"

class Menu {
public:
    Menu(int screenWidth = 128, int screenHeight = 64);
    ~Menu();
    void setRootItem(MenuItem* root);
    MenuItem* getCurrentItem();
    void action(MenuAction action);
    void displayMenu(Adafruit_SSD1306& display);
private:
    MenuItem* rootItem;
    MenuItem* currentItem;
    size_t selectedChildIndex;
    size_t scrollOffset;
    size_t screenWidth;
    size_t screenHeight;

    void navigateToChild(size_t index);
    void navigateToParent();
    void navigateUp();
    void navigateDown();
    void executeCurrentAction();
};


#endif