#include "alarmclock.h"

#include <SPI.h>  // For SPI comm (needed for not getting compile error)
#include <Wire.h> // For I2C comm (needed for not getting compile error)

STATE_t AlarmClk::currentState = INIT;
unsigned long AlarmClk::snoozeTimerReenableMillis = 0;

AlarmClk::AlarmClk() : console(BAUDRATE), oledDisplay(OLED_RESET_PIN, SNOOZE_TIME), motor(MOTOR_PIN)
{

    // currentState = INIT;

    pinMode(SNOOZE_BTN_PIN, INPUT); // Snooze
    pinMode(ALARM_BTN_PIN, INPUT);  // Alarm

    switchToState(WIFI_CONNECTING);
}

void AlarmClk::evaluateStateChange()
{
    // Depending on the currentState there are diffrent conditions for a state change
    switch (currentState)
    {
    case INIT:
        break;
    case FAULT:
        break;
    case WIFI_CONNECTING:
        if (wlan.isWifiConnected())
        {
            // Connection successful. Change to IDLE state and wait for new firebase message.
            switchToState(IDLE);
            Serial.println("Connected to network '" + wlan.getConnectedWifiName() + "'.");
            wlan.beginInternetServices();

            // Set lastMessageID to processed ID before device lost power. (Should always be the last message, if there has not really been another one)
            wlan.setLastMessageID(console.getLastMessageID()); // uint32_t cast to int => Will lose precision, but who cares lol (reach 4 bil messages first..)
            Serial.printf("Set lastMessageID to %d from EEPROM.", wlan.getLastMessageID());
        }
        break;
    case IDLE:
        if (!wlan.isWifiConnected())
        {
            // Jump to reconnect state
            switchToState(WIFI_RECONNECTING);
            break;
        }
        checkForNewMessage();

        break;
    case MESSAGE_ON_SCREEN:
        checkForNewMessage();
        break;
    case SLEEP:
    {
        // Reenable alarm after the 10 minutes have passed but only if not 0 (reenable timer active)
        if ((millis() - snoozeTimerReenableMillis > SNOOZE_TIME * 1000) && snoozeTimerReenableMillis != 0)
        {
            snoozeTimerReenableMillis = 0; // Disable the timer.
            oledDisplay.clearCounterInLeftCorner();
            switchToState(MESSAGE_ON_SCREEN);
        }
    }
    break;
    case WIFI_RECONNECTING:
        if (wlan.isWifiConnected())
        {
            switchToState(IDLE);
        }
        break;
    }
}

void AlarmClk::switchToState(STATE_t state)
{
    switch (state)
    {
    case INIT:
    { // Initialization of the program (unused)
    }
    break;
    case FAULT:
    { // TODO: Improve error handling
        assert(NULL);
    }
        return;
    case WIFI_CONNECTING:
    {
        // Read current stored wifi settings and check if there is a connection possible
        // Display no wifi symbol.
        oledDisplay.drawNoWifiSymbol();
        String ssid = console.getWifiSSIDFromMemory();
        String password = console.getWifiPasswordFromMemory();

        Serial.printf("\n\n==============================\nREMOTE ALARM CLK %s\nCommit %s\nCompiled on %s, %s\nMake sure, \\n and \\r\\n are DISABLED, before using commands. This may cause errors (with wifi credentials for example)\n==============================\n\n", SOFTWARE_VERSION, GIT_COMMIT_HASH, __DATE__, __TIME__);
        Serial.printf("\nTrying to connect to %s with password ***. \nWant to change WiFi credentials? Type (after first attempt): 'ssid:<newSSID>' or 'password:<newPassword>'\n\n Please wait", ssid.c_str());

        wlan.attemptWifiConnection(ssid, password, oledDisplay);
    }
    break;

    case IDLE:
    { // Connection successful (IDLE comes from WIFI_CONNECTING)
        oledDisplay.clearDisplay();
    }
    break;
    case MESSAGE_ON_SCREEN:
    {
        // if there is message: display it!
        snoozeTimerReenableMillis = 0; // Reset snooze just to be safe.
        enableMessage();
    }
    break;
    case SLEEP:
    {
    }
    break;
    case WIFI_RECONNECTING:
    {
        oledDisplay.drawNoWifiSymbol();
    }
    break;
    }

    currentState = state;
}

STATE_t AlarmClk::getCurrentStateOfProgram()
{
    return currentState;
}

unsigned long AlarmClk::getSnoozeTimerReenableMillis()
{
    return snoozeTimerReenableMillis;
}

void AlarmClk::loop()
{
    evaluateStateChange();

    console.userInputTick();
    wlan.networkTick();
    oledDisplay.renderTick();
    motor.motorFlickerTick(); // Motor flicker handler.

    // Don't run the code, if there is a setup problem with Wifi
    // FIXME: Reimplement fault when wifi connection has failed! (Force user to console or restart)
    // if (wlan.isProblemWithWifi())
    //    return;


    // The SNOOZE button has been pressed. Disable Alarm display SCHNUSCH and renable alarm after x time
    if (digitalRead(SNOOZE_BTN_PIN) == HIGH && (millis() - lastDebounceSnoozeMillis > DEBOUNCE_TIME) && messageOnScreen)
    {
        messageOnScreen = false; // Disable the on screen message
        lastDebounceSnoozeMillis = millis();
        Serial.println("Snooze Button has been pressed!");
        motor.changeMotorState(false); // Disable motor
        oledDisplay.splashSnooze();    // Display text
        wlan.sendMessageToFirebase("Komme 10 Minuten später!");
        snoozeTimerReenableMillis = millis(); // Enable snooze timer.
    }

    // Disable message and disable the reenable timer.
    if (digitalRead(ALARM_BTN_PIN) == HIGH && (millis() - lastDebounceStopMillis > DEBOUNCE_TIME) && (messageOnScreen || snoozeTimerReenableMillis != 0))
    {
        messageOnScreen = false; // Disable the on screen message
        lastDebounceStopMillis = millis();
        snoozeTimerReenableMillis = 0; // Disable reenable timer.
        oledDisplay.clearCounterInLeftCorner();
        motor.changeMotorState(false);
        oledDisplay.splashStop();
        wlan.sendMessageToFirebase("Fahre jetzt los!");
    }

    // OLed display
    //    show battery voltage and or if enabled
    //    show current message
    //    show current time (not necessary)
    //    flash random pixelart (from github)
    // !schnuusch (D7) pulldown disable the alarm for 7 minutes and send message over signal to chat ("Ich komme 10 Minuten spaeter.")
    // ausschalten (D6) pulldown disable the alarm permanently and send message over signal to chat ("Mache mich auf den Weg.")
    // !enable motor (D5)
    // !get wakeup time and message from github
    // !get additional messages with variable time from github
}

/**
 * Enables a message
 */
void AlarmClk::enableMessage()
{
    messageOnScreen = true;
    motor.changeMotorState(lastMotorStatus);
    oledDisplay.clearDisplay();
    oledDisplay.displayMessage(lastText);
    lastDebounceSnoozeMillis = 0; // Enable snooze button at once
}

void AlarmClk::checkForNewMessage()
{
    wlan.fetchNewMessagesFromFirebase();
    databaseObj_t msg = wlan.processNewMessagesFromFirebase();

    if (msg.messageID != -1)
    {
        bool success = console.writeLastMessageID((uint32_t)msg.messageID); // Can only be positive at this point!

        Serial.printf("Status EEPROM write of lastMessageID: %s", ((success ? "success" : "ERROR WITH WRITE!")));

        // Set values for enableMessage();
        lastText = msg.sender + "> " + msg.text;
        lastMotorStatus = msg.motorStatus;
        snoozeTimerReenableMillis = 0;

        switchToState(MESSAGE_ON_SCREEN);
    }
}