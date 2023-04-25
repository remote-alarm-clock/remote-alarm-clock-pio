#include "motor.h"

Motor::Motor(uint8_t motorPinTemp){
    motorPin = motorPinTemp;
    pinMode(motorPin, OUTPUT); // Motor Pin
}

/**
 * Handle the state changes to the motor
 */
void Motor::changeMotorState(bool enable)
{
  motorRunning = enable;
  lastMotorFlickerChangeMillis = (enable ? millis() : 0); // Enable flicker mode
  motorFlickerChangeState = enable;                       // Set flicker state to true (if enabled). Equivalent to = true, but current motor state is always carried over
  digitalWrite(motorPin, enable);
  Serial.printf("Changing state of motor to %s\n", (enable ? "running" : "halted"));
}

void Motor::loopMotorFlicker()
{
  if (lastMotorFlickerChangeMillis != 0 && motorRunning)
  {
    if ((millis() - lastMotorFlickerChangeMillis) > (motorFlickerChangeState ? MOTOR_FLICKER_TIME_ON : MOTOR_FLICKER_TIME_OFF)) // Select either on or off time
    {
      lastMotorFlickerChangeMillis = millis();
      motorFlickerChangeState =! motorFlickerChangeState; // Switch the mode
      digitalWrite(motorPin, motorFlickerChangeState);
    }
  }
}
