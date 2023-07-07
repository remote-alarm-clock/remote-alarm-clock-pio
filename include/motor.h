#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

#include "config.h"

class Motor
{
public:
    Motor(uint8_t motorPin);
    void changeMotorState(bool enable);
    void motorFlickerTick();

private:
    unsigned int motorPin;
    bool motorRunning = false;
    unsigned long lastMotorFlickerChangeMillis = 0; // Turn the motor on and off flicker
    bool motorFlickerChangeState = true;            // motor on and off flicker variable
};

#endif