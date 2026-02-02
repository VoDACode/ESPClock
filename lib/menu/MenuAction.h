#ifndef _MENU_ACTION_H_
#define _MENU_ACTION_H_

#include <Arduino.h>

enum MenuActionType {
    MENU_ACTION_NONE,
    MENU_ACTION_ROLL,
    MENU_ACTION_CLICK,
    MENU_ACTION_BACK
};

struct MenuAction
{
    MenuActionType type;
    int16_t value;

    MenuAction(MenuActionType type = MENU_ACTION_NONE, int16_t value = 0)
        : type(type), value(value)
    {
    }
};


#endif