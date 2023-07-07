#ifndef SIGCLK_H
#define SIGCLK_H

#include <Arduino.h>
//#include <Adafruit_GFX.h>

#include "config.h"
#include "console.h"
#include "oled.h"
#include "motor.h"
#include "wlan.h"

class AlarmClk
{
public:
    AlarmClk();
    void loop();

private:
    void enableMessage();
    void sendMessageToFirebase(String message);

    CONSOLE console;
    OLed oledDisplay;
    Motor motor;
    WLAN wlan;

    bool messageOnScreen = false;

    // Variables for Stop Function
    unsigned long lastDebounceStopMillis = 0;
    // Variables for Snooze Function
    unsigned long lastDebounceSnoozeMillis = 0;
    unsigned long snoozeTimerUpdateDisplayMillis = 0; // Last update of display
    unsigned long snoozeTimerReenableMillis = 0;
    String lastText = "";         // Latest message to display
    bool lastMotorStatus = false; // Latest motor status
};

#endif