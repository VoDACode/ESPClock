#include "menu.h"

MenuItem::MenuItem() : name(""), enable(true), parent(nullptr), action(nullptr), childCount(0), children(nullptr)
{
}

MenuItem::MenuItem(String name)
    : name(name), enable(true), parent(nullptr), action(nullptr), childCount(0), children(nullptr)
{
}

MenuItem::MenuItem(String name, void (*action)(), bool enable)
    : name(name), enable(enable), parent(nullptr), action(action), childCount(0), children(nullptr)
{
}

MenuItem::~MenuItem()
{
    delete[] children;
}

void MenuItem::addChild(MenuItem *child)
{
    if (child == nullptr)
        return;
    MenuItem *newChildren = new MenuItem[childCount + 1];
    for (size_t i = 0; i < childCount; i++)
    {
        newChildren[i] = children[i];
    }
    newChildren[childCount] = *child;
    newChildren[childCount].parent = this;
    delete[] children;
    children = newChildren;
    childCount++;
}

MenuItem *MenuItem::getChild(size_t index)
{
    if (index >= childCount || children == nullptr)
    {
        return nullptr;
    }
    return &children[index];
}

size_t MenuItem::getChildCount()
{
    return childCount;
}

Menu::Menu() : rootItem(nullptr), currentItem(nullptr), selectedChildIndex(0), scrollOffset(0)
{
}

Menu::~Menu()
{
    delete rootItem;
}

void Menu::setRootItem(MenuItem *root)
{
    rootItem = root;
    currentItem = root;
    selectedChildIndex = 0;
    scrollOffset = 0;
}

MenuItem *Menu::getCurrentItem()
{
    return currentItem;
}

void Menu::navigateToChild(size_t index)
{
    if (currentItem == nullptr)
        return;
    MenuItem *child = currentItem->getChild(index);
    if (child != nullptr && child->enable)
    {
        currentItem = child;
        selectedChildIndex = 0;
        scrollOffset = 0;
    }
}

void Menu::navigateToParent()
{
    if (currentItem != nullptr && currentItem->parent != nullptr)
    {
        currentItem = currentItem->parent;
        selectedChildIndex = 0;
        scrollOffset = 0;
    }
}

void Menu::navigateUp()
{
    if (currentItem == nullptr)
        return;
    
    size_t childCount = currentItem->getChildCount();
    if (childCount == 0)
        return;
    
    // Move selection up, skip disabled items
    size_t startIndex = selectedChildIndex;
    do
    {
        if (selectedChildIndex > 0)
        {
            selectedChildIndex--;
        }
        else
        {
            selectedChildIndex = childCount - 1; // Wrap around to bottom
        }
        
        MenuItem *child = currentItem->getChild(selectedChildIndex);
        if (child != nullptr && child->enable)
        {
            // Adjust scroll offset if needed
            const size_t MAX_VISIBLE_ITEMS = 6; // Leave room for title
            if (selectedChildIndex < scrollOffset)
            {
                scrollOffset = selectedChildIndex;
            }
            break;
        }
    } while (selectedChildIndex != startIndex);
}

void Menu::navigateDown()
{
    if (currentItem == nullptr)
        return;
    
    size_t childCount = currentItem->getChildCount();
    if (childCount == 0)
        return;
    
    // Move selection down, skip disabled items
    size_t startIndex = selectedChildIndex;
    do
    {
        if (selectedChildIndex < childCount - 1)
        {
            selectedChildIndex++;
        }
        else
        {
            selectedChildIndex = 0; // Wrap around to top
        }
        
        MenuItem *child = currentItem->getChild(selectedChildIndex);
        if (child != nullptr && child->enable)
        {
            // Adjust scroll offset if needed
            const size_t MAX_VISIBLE_ITEMS = 6; // Leave room for title
            if (selectedChildIndex >= scrollOffset + MAX_VISIBLE_ITEMS)
            {
                scrollOffset = selectedChildIndex - MAX_VISIBLE_ITEMS + 1;
            }
            break;
        }
    } while (selectedChildIndex != startIndex);
}

void Menu::executeCurrentAction()
{
    if (currentItem == nullptr)
        return;
    size_t childCount = currentItem->getChildCount();
    
    // If current item has children, navigate to selected child
    if (childCount > 0)
    {
        MenuItem *selectedChild = currentItem->getChild(selectedChildIndex);
        if (selectedChild != nullptr && selectedChild->enable)
        {
            // Execute child's action if it has one
            if (selectedChild->action != nullptr)
            {
                selectedChild->action();
            }
            // Navigate into the child if it has children
            if (selectedChild->getChildCount() > 0)
            {
                navigateToChild(selectedChildIndex);
            }
        }
    }
    // If current item has no children, execute its action
    else if (currentItem->action != nullptr)
    {
        currentItem->action();
    }
}

void Menu::displayMenu(Adafruit_SSD1306 &display)
{
    if (currentItem == nullptr)
        return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // Display current item name (title)
    display.println(currentItem->name);
    display.println(""); // Blank line for separation

    // Calculate visible items (64 pixels high, 8 pixels per line, 2 lines for title)
    const size_t MAX_VISIBLE_ITEMS = 6;
    size_t childCount = currentItem->getChildCount();
    
    if (childCount == 0)
    {
        display.println("  (no items)");
    }
    else
    {
        // Display visible child items based on scroll offset
        size_t endIndex = scrollOffset + MAX_VISIBLE_ITEMS;
        if (endIndex > childCount)
        {
            endIndex = childCount;
        }
        
        for (size_t i = scrollOffset; i < endIndex; i++)
        {
            MenuItem *child = currentItem->getChild(i);
            if (child != nullptr)
            {
                // Highlight selected item with inverse colors
                if (i == selectedChildIndex)
                {
                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Inverted
                    display.print(">");
                }
                else
                {
                    display.setTextColor(SSD1306_WHITE);
                    display.print(" ");
                }
                
                display.print(child->enable ? " " : "x ");
                display.println(child->name);
                
                // Reset colors
                display.setTextColor(SSD1306_WHITE);
            }
        }
        
        // Show scroll indicators
        if (scrollOffset > 0)
        {
            display.setCursor(120, 10);
            display.print("^");
        }
        if (endIndex < childCount)
        {
            display.setCursor(120, 56);
            display.print("v");
        }
    }

    display.display();
}