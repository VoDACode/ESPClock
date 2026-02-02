#include "menu.h"

Menu::Menu(
    int screenWidth,
    int screenHeight) : rootItem(nullptr), currentItem(nullptr), selectedChildIndex(0), scrollOffset(0),
                        screenWidth(screenWidth), screenHeight(screenHeight)
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

void Menu::action(MenuAction action)
{
    if(currentItem != nullptr && currentItem->onAction(action))
    {
        return;
    }
    switch (action.type)
    {
    case MENU_ACTION_ROLL:
        if (action.value > 0)
        {
            navigateDown();
        }
        else if (action.value < 0)
        {
            navigateUp();
        }
        break;
    case MENU_ACTION_CLICK:
        executeCurrentAction();
        break;
    case MENU_ACTION_BACK:
        navigateToParent();
        break;
    default:
        break;
    }
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
            // Adjust scroll offset if selected item is above visible area
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
            // Перевірити, чи вибраний елемент за межами видимої області
            // Підрахувати висоту всіх елементів від scrollOffset до selectedChildIndex
            int totalHeight = 16; // Висота заголовка
            bool needScroll = false;
            
            for (size_t i = scrollOffset; i <= selectedChildIndex; i++)
            {
                MenuItem *item = currentItem->getChild(i);
                if (item != nullptr)
                {
                    MenuItemSize itemSize = item->getSize();
                    totalHeight += itemSize.height;
                    
                    if (totalHeight > screenHeight)
                    {
                        needScroll = true;
                        break;
                    }
                }
            }
            
            if (needScroll && scrollOffset < selectedChildIndex)
            {
                scrollOffset++;
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
            // Execute child's onClick handler
            selectedChild->onClick();
            
            // Navigate into the child if it has children
            if (selectedChild->getChildCount() > 0)
            {
                navigateToChild(selectedChildIndex);
            }
        }
    }
    // If current item has no children, execute its onClick handler
    else
    {
        currentItem->onClick();
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
    display.println(currentItem->getName());
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
        int currentY = 16; // Початкова позиція після заголовка
        size_t visibleCount = 0;
        size_t startRenderIndex = scrollOffset;
        
        // Знайти, скільки елементів поміститься на екран
        for (size_t i = scrollOffset; i < childCount && currentY < screenHeight; i++)
        {
            MenuItem *child = currentItem->getChild(i);
            if (child != nullptr)
            {
                MenuItemSize itemSize = child->getSize();
                if (currentY + itemSize.height <= screenHeight)
                {
                    visibleCount++;
                }
            }
        }
        
        size_t endIndex = scrollOffset + visibleCount;
        if (endIndex > childCount)
        {
            endIndex = childCount;
        }
        
        currentY = 16; // Скинути для рендерингу
        for (size_t i = scrollOffset; i < endIndex; i++)
        {
            MenuItem *child = currentItem->getChild(i);
            if (child != nullptr)
            {
                MenuItemSize itemSize = child->getSize();
                
                // Перевірити, чи елемент поміститься
                if (currentY + itemSize.height > screenHeight)
                {
                    break;
                }
                
                int xPos = 0;
                
                // Індикатор вибору
                display.setCursor(xPos, currentY);
                display.setTextColor(SSD1306_WHITE);
                display.print(i == selectedChildIndex ? ">" : " ");
                
                // Індикатор стану (enabled/disabled)
                xPos += 6;
                display.setCursor(xPos, currentY);
                display.print(child->enable ? " " : "x");
                
                // Рендер самого елемента через віртуальний метод
                xPos += 12;
                child->render(display, xPos, currentY, i == selectedChildIndex);
                
                // Перейти до наступної позиції з урахуванням висоти елемента
                currentY += itemSize.height;
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