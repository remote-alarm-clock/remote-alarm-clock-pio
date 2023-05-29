#include "alarmclock.h"

#include <ESP8266WiFi.h>
#include <SPI.h>                // For SPI comm (needed for not getting compile error)
#include <Wire.h>               // For I2C comm (needed for not getting compile error)
#include <addons/TokenHelper.h> // Provide the token generation process info.
#include <addons/RTDBHelper.h>  // Provide the RTDB payload printing info and other helper functions.

AlarmClk::AlarmClk() : console(BAUDRATE), oledDisplay(OLED_RESET_PIN, SNOOZE_TIME), motor(MOTOR_PIN), timeClient(ntpUDP)
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

    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);

    // Try to connect
    // If connection after 5 seconds not successful, display on screen to change pw, break this routine
    // Clock should then be restarted for changes to work properly (boolean or something?)

    unsigned long millisConnectionAttemptStarted = millis();
    int frame = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        // Display connecting animation
        if (millis() > millisConnectionAttemptStarted + WIFI_CONNECTION_ATTEMPT_DURATION)
        {
            // Setup done.. There is no wifi. Show on screen.
            oledDisplay.drawSetupWifiConnection();
            problemWithWifi = true;
            Serial.println("\nNo connection to Wifi possible. Please make sure, ssid and password are correct!");
            return;
        }
        Serial.print(".");
        oledDisplay.drawWifiConnectionAttemptSymbol(frame % 3);
        if (((millis() - millisConnectionAttemptStarted) % 500) < 50) frame++;
        yield();
    }

    // Connection successful.
    oledDisplay.clearDisplay();

    Serial.println(" connected");

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    Firebase.begin(&config, &auth);

    // Comment or pass false value when WiFi reconnection will control by your code or third party library
    Firebase.reconnectWiFi(true);

    // // Recommend for ESP8266 stream, adjust the buffer size to match your stream data size
    // stream.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);

    // Set lastMessageID to processed ID before device lost power. (Should always be the last message, if there has not really been another one)
    lastMessageID = console.getLastMessageID(); // uint32_t cast to int => Will lose precision, but who cares lol (reach 4 bil messages first..)
    Serial.printf("Set lastMessageID to %d from EEPROM.", lastMessageID);

    timeClient.begin();
}

void AlarmClk::loop()
{
    timeClient.update();
    console.loopUserInput();

    // Don't run the code, if there is a setup problem with Wifi
    if (problemWithWifi)
        return;

    motor.loopMotorFlicker(); // Motor flicker handler.

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

    // Connect to database and read newest message status.
    if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
    {
        // check if there has been a new message
        sendDataPrevMillis = millis();
        bool success = Firebase.RTDB.getJSON(&fbdo, "/clocks/" + (String)CLOCK_ID + "/messages/latest_message_id");
        // Serial.printf("Get json... %s\n",  ? fbdo.to<FirebaseJson>().raw() : fbdo.errorReason().c_str());
        if (!success)
        {
            // report error on display
            Serial.printf("Error receiving data: %s", fbdo.errorReason().c_str());
            return;
        }

        int newestMessageID = fbdo.to<int>();

        if (newestMessageID != lastMessageID)
        {
            lastMessageID = newestMessageID;
            Serial.printf("Newest message ID %d\n\n", lastMessageID);
            dataChanged = true;
        }
    }

    // Handle any changes of data. (There has been a new message)
    if (dataChanged)
    {
        Serial.println("Data has changed!");
        // Read the newest message from Firebase
        if (Firebase.ready())
        {
            if (lastMessageID < 0)
            {
                Serial.println("NewestMessageID is smaller than 0. There has been an error!!");
                return;
            }
            if (!Firebase.RTDB.getJSON(&fbdo, "/clocks/" + (String)CLOCK_ID + "/messages/" + lastMessageID))
            {
                Serial.printf("Cannot fetch data from Firebase. %s\n", fbdo.errorReason().c_str());
                return;
            }
            FirebaseJson resultingMessage = fbdo.to<FirebaseJson>();
            FirebaseJsonData dat;
            resultingMessage.get(dat, "text");
            String text = dat.to<String>();
            resultingMessage.get(dat, "sender_name");
            String sender = dat.to<String>();
            resultingMessage.get(dat, "bell");
            bool bell = dat.to<bool>();

            Serial.printf("Message received and processed: %s: %s, bell: %s \n", sender.c_str(), text.c_str(), (bell ? "yes" : "no"));

            bool success = console.writeLastMessageID((uint32_t)lastMessageID); // Can only be positive at this point!

            Serial.printf("Status EEPROM write of lastMessageID: %s", ((success ? "success" : "ERROR WITH WRITE!")));

            // Set values for enableMessage();
            lastText = sender + "> " + text;
            lastMotorStatus = bell;
            snoozeTimerReenableMillis = 0;

            enableMessage();
        }
        dataChanged = false;
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

void AlarmClk::sendMessageToFirebase(String message)
{
    if (Firebase.ready())
    {
        // Read current message ID
        Firebase.RTDB.getJSON(&fbdo, "/clocks/" + (String)CLOCK_ID + "/clock_fb/latest_clock_status_count");
        int i = fbdo.to<int>();
        delay(500);
        // Update to a newer message!
        FirebaseJson json;
        json.add("latest_clock_status_count", i + 1);
        json.add("latest_clock_status", message);
        json.add("latest_clock_status_utc", timeClient.getEpochTime());
        if (!Firebase.RTDB.setJSON(&fbdo, "/clocks/" + (String)CLOCK_ID + "/clock_fb", &json))
        {
            Serial.printf("Could not send status message! %s \n", fbdo.errorReason().c_str());
        }
        else
        {
            Serial.printf("Sent '%s' to Firebase!\n", message.c_str());
        }
    }
    else
        Serial.println("Firebase is not ready. Cannot send message!");
}