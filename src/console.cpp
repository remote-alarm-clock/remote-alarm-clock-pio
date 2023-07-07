#include "console.h"

#include <EEPROM.h>

CONSOLE::CONSOLE(unsigned long baudrate)
{
    Serial.begin(baudrate);
    // 0:len ssid, 1-255: ssid, 256: len pass, 257-511: pass, 512-515: lastMessageID
    EEPROM.begin(1024); // Initialize EEPROM to a size of 1kbytes
}

void CONSOLE::userInputTick()
{
    if (Serial.available() > 0)
    {
        String message = Serial.readString();
        if (message.startsWith("ssid:"))
        {
            // Change the SSID of Network.
            String ssidTemp = message.substring(5);
            int len = ssidTemp.length();

            if (len > 254)
            {
                Serial.printf("'%s' is too long. Maximum SSID length is 254 characters (non-ascii characters count as two)\n", ssidTemp.c_str());
                return;
            }

            if (writeWifiSSIDToMemory(ssidTemp, (byte)len))
            {
                Serial.printf("Changed WiFi SSID to '%s'. Remember to change password also.\n", ssidTemp.c_str());
                return;
            }
            else
            {
                Serial.printf("Error storing ssid. Memory broken. Maybe use a new ESP?\n");
                return;
            }
        }
        else if (message.startsWith("password:"))
        {
            // Change the SSID of Network.
            String passwordTemp = message.substring(9);
            int len = passwordTemp.length();
            if (len > 254)
            {
                Serial.printf("'%s' is too long. Maximum password length is 254 characters (non-ascii characters count as two)\n", passwordTemp.c_str());
                return;
            }
            if (writeWifiPasswordToMemory(passwordTemp, (byte)len))
            {
                Serial.printf("Changed WiFi password to '%s'. Remember to change SSID also.\n", passwordTemp.c_str());
                return;
            }
            else
            {
                Serial.printf("Error storing password. Memory broken. Maybe use a new ESP?\n");
                return;
            }
        }
        else if(message.equals("restart")){
            Serial.println("Rebooting your CLOCK_ID in 3s.");
            delay(3000);
            ESP.restart();
            return;
        }
        else
        {
            Serial.printf("Command '%s' is unknown. Try 'ssid:<newSSID>' or 'password:<newPassword>'!\n", message.c_str());
        }
    }
}

String CONSOLE::getWifiPasswordFromMemory()
{
    // byte length, max 254 bytes content
    int newStrLenPassword = EEPROM.read(256);
    char dataPassword[newStrLenPassword + 1];
    for (int i = 0; i < newStrLenPassword; i++)
    {
        dataPassword[i] = EEPROM.read(256 + 1 + i);
    }
    dataPassword[newStrLenPassword] = '\0';
    return String(dataPassword);
}
String CONSOLE::getWifiSSIDFromMemory()
{
    // byte length, max 254 bytes content
    int newStrLenSSID = EEPROM.read(0);
    char dataSSID[newStrLenSSID + 1];
    for (int i = 0; i < newStrLenSSID; i++)
    {
        dataSSID[i] = EEPROM.read(1 + i);
    }
    dataSSID[newStrLenSSID] = '\0';
    return String(dataSSID);
}

bool CONSOLE::writeWifiSSIDToMemory(String ssid, byte length)
{
    // Store ssid in memory
    EEPROM.write(0, length);
    for (int i = 0; i < length; i++)
    {
        EEPROM.write(1 + i, ssid[i]);
    }
    return EEPROM.commit();
}
bool CONSOLE::writeWifiPasswordToMemory(String password, byte length)
{
    // Store password in memory
    EEPROM.write(256, length);
    for (int i = 0; i < length; i++)
    {
        EEPROM.write(256 + 1 + i, password[i]);
    }
    return EEPROM.commit();
}

uint32_t CONSOLE::getLastMessageID()
{
    // big endian MSB
    uint32_t lastMessageID = 0;

    for (int i = 0; i < 4; i++)
    {
        lastMessageID = (lastMessageID << (i * 8)) | EEPROM.read(512 + i);
    }

    return lastMessageID;
}

bool CONSOLE::writeLastMessageID(uint32_t lastMessageID)
{
    // Address: 512-515
    // lastMessageID >> 3 * 8  (& 0xFF)
    // lastMessageID >> 2 * 8   & 0xFF
    // lastMessageID >> 1 * 8   & 0xFF
    // lastMessageID >> 0       & 0xFF

    // big endian MSB
    for (int i = 0; i < 4; i++)
    {
        EEPROM.write(512 + i, lastMessageID >> (3 - i) & 0xFF);
    }
    return EEPROM.commit();
}