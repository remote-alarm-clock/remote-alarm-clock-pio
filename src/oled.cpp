#include "oled.h"

OLed::OLed(int oledResetPin, const int snoozeTimeVar) : display(oledResetPin), snoozeTime(snoozeTimeVar)
{
  display.begin(SSD1306_SWITCHCAPVCC);
}

void OLed::clearCounterInLeftCorner()
{
  display.fillRect(0, 0, 29, 7, BLACK);
  display.display();
}

/**
 * Note: external check if timer is enabled should be made.
 */
void OLed::updateCounterInLeftCorner(int snoozeTimerReenableMillis)
{
  // Snooze will only be max. 10 minutes
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  unsigned int min = floor((snoozeTime - (millis() - snoozeTimerReenableMillis) / 1000) / 60);
  unsigned int seconds = floor((snoozeTime - ((millis() - snoozeTimerReenableMillis) / 1000)) % 60);
  display.fillRect(0, 0, 29, 7, BLACK);
  display.printf("%02d:%02d", min, seconds); // 09:15
  display.display();
}

void OLed::splashStop()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  // position text in middle

  display.setCursor(17, 25);
  // 7 high 5 wide times TextSize in Pixels
  display.println("STOP");
  display.display();
  for (int i = 0; i < 4; i++)
  {
    display.invertDisplay(i % 2);
    delay(400);
  }
  display.invertDisplay(false);
  clearDisplay();
}

void OLed::splashSnooze()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  // position text in middle

  display.setCursor(17, 25);
  // 7 high 5 wide times TextSize in Pixels
  display.println("SCHNUSCH");
  display.display();
  for (int i = 0; i < 4; i++)
  {
    display.invertDisplay(i % 2);
    delay(400);
  }
  display.invertDisplay(false);
  clearDisplay();
}

void OLed::drawTwelve()
{
  // 48 wide 36 high
  // Centered and in first row
  display.setCursor(0, 0);
  display.drawBitmap(40, 0, twelve_clock, 48, 36, 1);
  display.display();
}

void OLed::clearDisplay()
{
  display.clearDisplay();
  drawTwelve();
}

void OLed::displayMessage(String mes)
{
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 44);
  display.println(mes);
  display.display();
}

void OLed::drawNoWifiSymbol()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.drawBitmap(40, 0, wifi_slash, 48, 36, 1);
  display.display();
}

void OLed::drawSetupWifiConnection()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.drawBitmap(40, 0, wifi_error, 48, 36, 1);
  display.setCursor(0, 44);
  display.setTextColor(WHITE);
  display.println("No Wifi connection possible. Setup via Serial!");
  display.display();
}

void OLed::drawWifiConnectionAttemptSymbol(int frame)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  switch (frame)
  {
  case 0:
    display.drawBitmap(40, 0, wifi_solid_1, 48, 36, 1);
    break;
  case 1:
    display.drawBitmap(40, 0, wifi_solid_2, 48, 36, 1);
    break;
  case 2:
    display.drawBitmap(40, 0, wifi_solid, 48, 36, 1);
    break;
  default:
    display.setTextColor(WHITE);
    display.println("Unknown frame ID!");
    break;
  }
  display.display();
}
void OLed::testfillrect()
{
  uint8_t color = 1;
  for (int16_t i = 0; i < display.height() / 2; i += 3)
  {
    // alternate colors
    display.fillRect(i, i, display.width() - i * 2, display.height() - i * 2, color % 2);
    display.display();
    color++;
  }
}
