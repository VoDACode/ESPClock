#include "button.h"

Button::Button(uint8_t pin, bool activeLow, unsigned long debounceDelay)
    : pin(pin),
      activeLow(activeLow),
      debounceDelay(debounceDelay),
      lastDebounceTime(0),
      lastButtonState(false),
      buttonState(false),
      pressedEvent(false),
      releasedEvent(false),
      pressStartTime(0),
      clickCounter(0),
      firstClickTime(0),
      lastClickTime(0) {}

void Button::begin()
{
    pinMode(pin, INPUT);
}

void Button::update()
{
    bool reading = digitalRead(pin);
    if (activeLow)
    {
        reading = !reading;
    }

    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != buttonState)
        {
            buttonState = reading;

            if (buttonState)
            {
                pressedEvent = true;
                pressStartTime = millis();

                // Click handling
                if (clickCounter == 0)
                {
                    firstClickTime = millis();
                }
                clickCounter++;
                lastClickTime = millis();
                
                // Якщо забагато кліків (більше 10) - скидаємо
                if (clickCounter > 10)
                {
                    clickCounter = 1;
                    firstClickTime = millis();
                }
            }
            else
            {
                releasedEvent = true;
            }
        }
    }

    lastButtonState = reading;
}

bool Button::isPressed()
{
    return buttonState;
}

bool Button::wasPressed()
{
    bool wasPressed = pressedEvent;
    pressedEvent = false;
    return wasPressed;
}

bool Button::wasReleased()
{
    bool wasReleased = releasedEvent;
    releasedEvent = false;
    return wasReleased;
}

bool Button::isLongPressed(unsigned long longPressDuration)
{
    if (buttonState && (millis() - pressStartTime >= longPressDuration))
    {
        return true;
    }
    return false;
}

bool Button::wasClickedNTimes(uint8_t clickCount, unsigned long timeWindow)
{
    // Якщо немає кліків - відразу повертаємо false
    if (clickCounter == 0)
    {
        return false;
    }
    
    // Якщо не пройшов час після останнього кліка - ще чекаємо
    if ((millis() - lastClickTime) < timeWindow)
    {
        return false;
    }
    
    // Час вийшов, перевіряємо чи співпадає кількість кліків
    if (clickCounter == clickCount)
    {
        clickCounter = 0; // Скидаємо тільки якщо знайшли потрібну кількість
        return true;
    }
    
    // Кількість кліків не співпадає - НЕ скидаємо, щоб інші виклики могли перевірити
    return false;
}