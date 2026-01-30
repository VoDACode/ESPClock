#include "StatusBar.h"

StatusBar::StatusBar(Adafruit_SSD1306 &display) : display(display)
{
}

StatusBar::Element::Element()
    : text("?"), type(STATUS_BAR_ELEMENT_TEXT)
{
}

StatusBar::Element::Element(String text)
    : text(text), type(STATUS_BAR_ELEMENT_TEXT)
{
}

StatusBar::Element::Element(const unsigned char* img, int width, int height)
    : img(img), width(width), height(height), type(STATUS_BAR_ELEMENT_IMG)
{
}

StatusBar::Element::Element(const Element &other)
{
    type = other.type;
    side = other.side;
    text = other.text;
    img = other.img;
    width = other.width;
    height = other.height;
    visible = other.visible;
}

void StatusBar::Element::draw(Adafruit_SSD1306 &display, int x, int y)
{
    if (type == STATUS_BAR_ELEMENT_TEXT)
    {
        display.setCursor(x, y);
        display.print(text);
    }
    else if (type == STATUS_BAR_ELEMENT_IMG && img != nullptr)
    {
        display.drawBitmap(x, y, img, width, height, SSD1306_WHITE);
    }
}

void StatusBar::addElement(Element *element)
{
    Element** newElements = new Element*[elementCount + 1];
    for (int i = 0; i < elementCount; i++)
    {
        newElements[i] = elements[i];
    }
    newElements[elementCount] = element;
    delete[] elements;
    elements = newElements;
    elementCount++;
}

void StatusBar::draw()
{
    int xLeft = 0;
    int xRight = display.width();
    int y = 0; // Малюємо у верхній частині екрану

    display.setTextSize(1);
    for (int i = 0; i < elementCount; i++)
    {
        Element *element = elements[i];

        if (!element->visible)
            continue;

        if (element->inverted) {
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }

        if (element->side == Element::STATUS_BAR_ELEMENT_LEFT)
        {
            element->draw(display, xLeft, y);
            if (element->type == Element::STATUS_BAR_ELEMENT_TEXT)
            {
                int16_t x1, y1;
                uint16_t w, h;
                display.getTextBounds(element->text.c_str(), xLeft, y, &x1, &y1, &w, &h);
                xLeft += w;
            }
            else if (element->type == Element::STATUS_BAR_ELEMENT_IMG)
            {
                xLeft += element->width;
            }
        }
        else // RIGHT
        {
            if (element->type == Element::STATUS_BAR_ELEMENT_TEXT)
            {
                int16_t x1, y1;
                uint16_t w, h;
                display.getTextBounds(element->text.c_str(), 0, 0, &x1, &y1, &w, &h);
                xRight -= w;
                element->draw(display, xRight, y);
            }
            else if (element->type == Element::STATUS_BAR_ELEMENT_IMG)
            {
                xRight -= element->width;
                element->draw(display, xRight, y);
            }
        }
    }
}