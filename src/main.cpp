/**
 * @file main.cpp
 * @author Aleksander Stepien
 *
 *
 */

#include "alarmclock.h"


/*TODO:
- !WLAN Symbol hinzufügen
- !WLAN setzen während Betrieb
- Nachricht in groß anzeigen
- Getimte Nachrichten (not important)
- Unicode blocken / äöü übersetzen..
- Allen Nutzern die letzte gesendete Nachricht anzeigen (Letzte Nachricht zu der Antwort zuorden APP)
- Option an alle senden / pn (APP)
- Timestamp in Nachrichten
- Benachrichtung wenn Netzteil abgezogen wird
- Batteriesymbol oben in Ecke
- Vernünftiges Errorlogging (Fehlerzeichen in Ecke bei Fehler in Console)
*/

// /**
//  * Handle changes of database of messages!
//  * https://github.com/mobizt/Firebase-ESP-Client/blob/main/examples/RTDB/DataChangesListener/Callback/Callback.ino
//  */

AlarmClk *clkPtr = nullptr;

void setup()
{
  clkPtr = new AlarmClk();
}

void loop()
{
  clkPtr->loop();
}
