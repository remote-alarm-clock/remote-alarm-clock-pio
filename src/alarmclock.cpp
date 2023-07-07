#include "alarmclock.h"

#include <SPI.h>  // For SPI comm (needed for not getting compile error)
#include <Wire.h> // For I2C comm (needed for not getting compile error)

AlarmClk::AlarmClk() : console(BAUDRATE), oledDisplay(OLED_RESET_PIN, SNOOZE_TIME), motor(MOTOR_PIN)
{

    pinMode(SNOOZE_BTN_PIN, INPUT); // Snooze
    pinMode(ALARM_BTN_PIN, INPUT);  // Alarm

    // Read current stored wifi settings and check if there is a connection possible
    // Display no wifi symbol.
    oledDisplay.drawNoWifiSymbol();

    String ssid = console.getWifiSSIDFromMemory();
    String password = console.getWifiPasswordFromMemory();

    delay(5000);
    Serial.printf("\n\n==============================\nREMOTE ALARM CLK %s\nCommit %s\nCompiled on %s, %s\nMake sure, \\n and \\r\\n are DISABLED, before using commands. This may cause errors (with wifi credentials for example)\n==============================\n\n", SOFTWARE_VERSION, GIT_COMMIT_HASH, __DATE__, __TIME__);
    Serial.printf("\nTrying to connect to %s with password ***. \nWant to change WiFi credentials? Type (after first attempt): 'ssid:<newSSID>' or 'password:<newPassword>'\n\n Please wait", ssid.c_str());

    wlan.attemptWifiConnection(ssid, password, oledDisplay);

    // Connection successful.
    oledDisplay.clearDisplay();

    Serial.println(" connected");

    wlan.beginInternetServices();

    // Set lastMessageID to processed ID before device lost power. (Should always be the last message, if there has not really been another one)
    wlan.setLastMessageID(console.getLastMessageID()); // uint32_t cast to int => Will lose precision, but who cares lol (reach 4 bil messages first..)
    Serial.printf("Set lastMessageID to %d from EEPROM.", wlan.getLastMessageID());
}

void AlarmClk::loop()
{
    console.userInputTick();

    wlan.networkTick();

    // Don't run the code, if there is a setup problem with Wifi
    if (wlan.isProblemWithWifi())
        return;

    motor.motorFlickerTick(); // Motor flicker handler.

    if ((millis() - snoozeTimerUpdateDisplayMillis) > DISPLAY_FRAMERATE_DELAY && snoozeTimerReenableMillis != 0)
    {
        snoozeTimerUpdateDisplayMillis = millis();
        // Update display showing counter timer.
        oledDisplay.updateCounterInLeftCorner(snoozeTimerReenableMillis);
    }
    // Reenable alarm after the 10 minutes have passed but only if not 0 (reenable timer active)
    if ((millis() - snoozeTimerReenableMillis > SNOOZE_TIME * 1000) && snoozeTimerReenableMillis != 0)
    {
        snoozeTimerReenableMillis = 0; // Disable the timer.
        oledDisplay.clearCounterInLeftCorner();
        enableMessage();
    }

    // The SNOOZE button has been pressed. Disable Alarm display SCHNUSCH and renable alarm after x time
    if (digitalRead(SNOOZE_BTN_PIN) == HIGH && (millis() - lastDebounceSnoozeMillis > DEBOUNCE_TIME) && messageOnScreen)
    {
        messageOnScreen = false; // Disable the on screen message
        lastDebounceSnoozeMillis = millis();
        Serial.println("Snooze Button has been pressed!");
        motor.changeMotorState(false); // Disable motor
        oledDisplay.splashSnooze();    // Display text
        sendMessageToFirebase("Komme 10 Minuten später!");
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
        sendMessageToFirebase("Fahre jetzt los!");
    }

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

        enableMessage();
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

