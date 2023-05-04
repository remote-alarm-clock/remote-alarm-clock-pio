#include "credentials.h"

#define CLOCK_ID "clock_1"
#define SOFTWARE_VERSION "v1.1"

#define DISPLAY_FRAMERATE_DELAY 500 // 2 FPS

#define MOTOR_FLICKER_TIME_ON 1500  // in millis
#define MOTOR_FLICKER_TIME_OFF 4000 // in millis

#define DEBOUNCE_TIME 1500 // Set to 1,5 seconds because we dont want to spam the database accidentally

#define SNOOZE_TIME 600 // in seconds!!

#define BAUDRATE 115200
#define WIFI_CONNECTION_ATTEMPT_DURATION 150000 // in millis smaller than 500ms steps doesnt make a difference

// Physical configuration
#define MOTOR_PIN D5
#define OLED_RESET_PIN D0 // pin 15 -RESET digital signal
#define SNOOZE_BTN_PIN D7
#define ALARM_BTN_PIN D6

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "?"
#endif
