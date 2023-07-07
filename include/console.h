#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>

class CONSOLE
{
public:
    CONSOLE(unsigned long baudrate);

    String getWifiPasswordFromMemory();
    String getWifiSSIDFromMemory();
    uint32_t getLastMessageID();
    

    bool writeLastMessageID(uint32_t lastMessageID);

    void userInputTick();

private:
    /**
     * persist Wifi password to EEPROM (only 254 characters allocated) 
     * 
     * @return true if successful write to memory
    */
    bool writeWifiPasswordToMemory(String password, byte length);

    /**
     * persist Wifi ssid to EEPROM (only 254 characters allocated)
     * 
     * @return true if successful wirte to memory
    */
    bool writeWifiSSIDToMemory(String ssid, byte length);
};

#endif