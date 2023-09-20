#ifndef SIGCLK_H
#define SIGCLK_H

#include <Arduino.h>
// #include <Adafruit_GFX.h>

#include "config.h"
#include "console.h"
#include "oled.h"
#include "motor.h"
#include "wlan.h"

// All possible states, that the programm can exist in.
typedef enum STATE_t
{
    INIT,
    FAULT,
    WIFI_CONNECTING,
    IDLE,
    MESSAGE_ON_SCREEN,
    SLEEP,
    WIFI_RECONNECTING
} STATE_t;
class AlarmClk
{
public:
    AlarmClk();

    /// @brief Main tick of the application
    void loop();
    /// @brief Check if conditions match to change a state.
    void evaluateStateChange();
    /// @brief Switch STM to new state (change state and setup parameters)
    void switchToState(STATE_t);
    /// @return in what state the STM is currently in
    static STATE_t getCurrentStateOfProgram();

    /// @brief Fetch data from Firebase (if ready) and evaluate if the data is new. If so, change the state to MESSAGE_ON_SCREEN.
    void checkForNewMessage();

    static unsigned long getSnoozeTimerReenableMillis();

private:
    void enableMessage();

    CONSOLE console;
    OLed oledDisplay;
    Motor motor;
    WLAN wlan;

    static STATE_t currentState;

    bool messageOnScreen = false;

    // Variables for Stop Function
    unsigned long lastDebounceStopMillis = 0;
    // Variables for Snooze Function
    unsigned long lastDebounceSnoozeMillis = 0;
    static unsigned long snoozeTimerReenableMillis;
    String lastText = "";         // Latest message to display
    bool lastMotorStatus = false; // Latest motor status
};

#endif