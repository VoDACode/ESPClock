#ifndef BUTTON_H
#define BUTTON_H
#include <Arduino.h>

class Button
{
public:
    Button(uint8_t pin, bool activeLow = true, unsigned long debounceDelay = 50);
    void begin();
    void update();
    bool isPressed();
    bool wasPressed();
    bool wasReleased();
    bool isLongPressed(unsigned long longPressDuration = 1000);
    bool wasClickedNTimes(uint8_t clickCount, unsigned long timeWindow = 500);

private:
    uint8_t pin;
    bool activeLow;
    unsigned long debounceDelay;
    unsigned long lastDebounceTime;
    bool lastButtonState;
    bool buttonState;
    bool pressedEvent;
    bool releasedEvent;
    unsigned long pressStartTime;
    uint8_t clickCounter;
    unsigned long firstClickTime;
    unsigned long lastClickTime;
};

#endif // BUTTON_H
