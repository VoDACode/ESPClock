#include "MenuItemContainer.h"

MenuItemContainer::MenuItemContainer(String name, bool enable)
    : MenuItem(name, nullptr, enable), childCount(0), children(nullptr)
{
    calculateSize();
}

MenuItemContainer::~MenuItemContainer()
{
    for (size_t i = 0; i < childCount; i++)
    {
        delete children[i];
    }
    delete[] children;
}

void MenuItemContainer::addChild(MenuItem* child)
{
    if (child == nullptr)
        return;
    
    MenuItem** newChildren = new MenuItem*[childCount + 1];
    
    for (size_t i = 0; i < childCount; i++)
    {
        newChildren[i] = children[i];
    }
    
    newChildren[childCount] = child;
    child->parent = this;
    
    delete[] children;
    children = newChildren;
    childCount++;
}

MenuItem* MenuItemContainer::getChild(size_t index)
{
    if (index >= childCount || children == nullptr)
    {
        return nullptr;
    }
    return children[index];
}

size_t MenuItemContainer::getChildCount()
{
    return childCount;
}

void MenuItemContainer::calculateSize()
{
    size.width = name.length() * 6 + 12; // + стрілка ">"
    size.height = 8;
}

void MenuItemContainer::render(Adafruit_SSD1306& display, int x, int y, bool selected)
{
    display.setCursor(x, y);
    
    if (selected)
    {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }
    else
    {
        display.setTextColor(SSD1306_WHITE);
    }
    
    display.print(name);
    display.print(" >");
    
    display.setTextColor(SSD1306_WHITE);
}

void MenuItemContainer::onClick()
{
    MenuItem::onClick();
}
