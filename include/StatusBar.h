#ifndef SYATUS_BAR_H
#define SYATUS_BAR_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

class StatusBar
{
public:
    struct Element
    {
        enum Type{
            STATUS_BAR_ELEMENT_TEXT,
            STATUS_BAR_ELEMENT_IMG
        };
        enum Side{
            STATUS_BAR_ELEMENT_LEFT,
            STATUS_BAR_ELEMENT_RIGHT
        };
        Type type = Type::STATUS_BAR_ELEMENT_TEXT;
        Side side = Side::STATUS_BAR_ELEMENT_LEFT;
        String text;
        const unsigned char* img = nullptr;
        int width = 0;
        int height = 0;
        bool visible = true;
        bool inverted = false;
        Element();
        Element(String text);
        Element(const unsigned char* img, int width, int height);
        Element(const Element &other);

        void draw(Adafruit_SSD1306 &display, int x, int y);
    };
    

    StatusBar(Adafruit_SSD1306 &display);
    void draw();
    void addElement(Element *element);
private:
    Adafruit_SSD1306 &display;
    int elementCount = 0;
    Element** elements = nullptr;  // Масив вказівників замість копій
};

#endif // SYATUS_BAR_H