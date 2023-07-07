#include "wlan.h"

#include <addons/TokenHelper.h> // Provide the token generation process info.
#include <addons/RTDBHelper.h>  // Provide the RTDB payload printing info and other helper functions.

WLAN::WLAN() : timeClient(ntpUDP), problemWithWiFi(false)
{
}

void WLAN::attemptWifiConnection(String ssid, String password, OLed &oledPtr)
{

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
            oledPtr.drawSetupWifiConnection();
            problemWithWiFi = true;
            Serial.println("\nNo connection to Wifi possible. Please make sure, ssid and password are correct!");
            return;
        }
        Serial.print(".");
        oledPtr.drawWifiConnectionAttemptSymbol(frame % 3);
        if (((millis() - millisConnectionAttemptStarted) % 500) < 50)
            frame++;
        yield();
    }
}

void WLAN::beginInternetServices()
{
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

    timeClient.begin();
}

void WLAN::setLastMessageID(int lastMSGID)
{
    lastMessageID = lastMSGID;
}

int WLAN::getLastMessageID()
{
    return lastMessageID;
}

void WLAN::networkTick()
{
    timeClient.update();
}

void WLAN::fetchNewMessagesFromFirebase()
{
    // Connect to database and read newest message status.
    if (Firebase.ready() && (millis() - sendDataPrevMillis > FIREBASE_POLLING_INTERVAL || sendDataPrevMillis == 0))
    {
        sendDataPrevMillis = millis();
        // check if there has been a new message
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
}

databaseObj_t WLAN::processNewMessagesFromFirebase()
{
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

            databaseObj_t message;
            message.messageID = lastMessageID;
            message.motorStatus = bell;
            message.sender = sender;
            message.text = text;

            lastMessage = message;
            dataChanged = false;
            return message;
        }
    }
    return dummyMessage;
}

void WLAN::sendMessageToFirebase(String message)
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

bool WLAN::isProblemWithWifi()
{
    return problemWithWiFi;
}