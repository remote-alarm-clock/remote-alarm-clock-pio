#ifndef WLAN_H
#define WLAN_H

#include "config.h"
#include "oled.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Firebase_ESP_Client.h>

typedef struct databaseObj_t
{
    String sender;
    String text;
    bool motorStatus;
    int messageID = -1;
} databaseObj_t;
class WLAN
{
public:
    WLAN();
    // Try to connect to internet
    void attemptWifiConnection(String, String, OLed &);
    // Start all services requiring an internet connection (NTPClient and Firebase)
    void beginInternetServices();
    void networkTick();
    void fetchNewMessagesFromFirebase();
    databaseObj_t processNewMessagesFromFirebase();
    void sendMessageToFirebase(String);

    void setLastMessageID(int);
    int getLastMessageID();
    String getConnectedWifiName();
    bool isWifiConnected();
    bool isProblemWithWifi();

private:
    WiFiUDP ntpUDP;
    NTPClient timeClient;

    // Define Firebase data object
    FirebaseData stream;
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;

    databaseObj_t lastMessage;
    databaseObj_t dummyMessage;

    String wifiName;
    unsigned long millisConnectionAttemptStarted = 0;

    int lastMessageID = -1;
    unsigned long sendDataPrevMillis;
    bool problemWithWiFi;
    bool dataChanged;
};

#endif