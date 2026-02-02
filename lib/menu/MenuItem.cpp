#include "MenuItem.h"

MenuItem::MenuItem(String name) : enable(true), parent(nullptr), action(nullptr), name(name)
{
    size.width = 0;
    size.height = 8;
}

MenuItem::MenuItem(String name, void (*action)(), bool enable)
    : enable(enable), parent(nullptr), action(action), name(name)
{
    size.width = 0;
    size.height = 8;
}

void MenuItem::calculateSize()
{
    size.width = 0;
    size.height = 8;
}

void MenuItem::onClick()
{
    if (action != nullptr)
    {
        action();
    }
}

bool MenuItem::onAction(MenuAction action)
{
    return false;
}

MenuItem::~MenuItem()
{
}

MenuItem *MenuItem::getChild(size_t index)
{
    return nullptr;
}

size_t MenuItem::getChildCount()
{
    return 0;
}

String MenuItem::getName()
{
    return name;
}

MenuItemSize MenuItem::getSize()
{
    return size;
}