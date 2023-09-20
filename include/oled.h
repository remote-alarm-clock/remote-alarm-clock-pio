#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <ESP_SSD1306.h>

#include "graphics.h"

class OLed
{
public:
    OLed(int oledResetPin, const int snoozeTime);
    void renderTick();
    void updateCounterInLeftCorner(int snoozeTimerReenableMillis);
    void clearCounterInLeftCorner();
    void clearDisplay();
    void drawTwelve();
    void drawNoWifiSymbol();
    void drawWifiConnectionAttemptSymbol(int frame); // Maximum 4 frames
    void drawSetupWifiConnection();
    void displayMessage(String);
    void splashSnooze();
    void splashStop();
    void testfillrect();

private:
    ESP_SSD1306 display; // FOR I2C
    const int snoozeTime;

    unsigned long millisSinceLastWLANSymbolUpdate;
    unsigned long millisSinceLastSnoozeDisplayUpdate;

    uint16_t wlanFrameNumber;
};
#endif