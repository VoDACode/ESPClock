#ifndef _MENU_H_
#define _MENU_H_

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

struct MenuItem {
public: 
    String name;
    bool enable;
    MenuItem* parent;
    void (*action)();
    void addChild(MenuItem* child);
    MenuItem* getChild(size_t index);
    size_t getChildCount();

    MenuItem();
    MenuItem(String name);
    MenuItem(String name, void (*action)(), bool enable = true);
    ~MenuItem();
private:
    size_t childCount;
    MenuItem* children;
};

class Menu {
public:
    Menu();
    ~Menu();
    void setRootItem(MenuItem* root);
    MenuItem* getCurrentItem();
    void navigateToChild(size_t index);
    void navigateToParent();
    void navigateUp();
    void navigateDown();
    void executeCurrentAction();
    void displayMenu(Adafruit_SSD1306& display);
private:
    MenuItem* rootItem;
    MenuItem* currentItem;
    size_t selectedChildIndex;
    size_t scrollOffset;
};


#endif