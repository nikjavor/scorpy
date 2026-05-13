#include <Arduino.h>
#include <ESP32Servo.h>

#include "board_pins.h"
#include "servo_display.h"

namespace
{
  Servo servoHomeOnes;
  Servo servoHomeTens;
  Servo servoAwayOnes;
  Servo servoAwayTens;

  int lastServoHome = -1;
  int lastServoAway = -1;

  constexpr int DIGIT_ANGLES[10] = {
      0, 20, 40, 60, 80, 100, 120, 140, 160, 180};
}

void beginServos()
{
  servoHomeOnes.attach(PIN_SERVO_HO);
  servoHomeTens.attach(PIN_SERVO_HT);
  servoAwayOnes.attach(PIN_SERVO_AO);
  servoAwayTens.attach(PIN_SERVO_AT);
}

void updateServos(int scoreHome, int scoreAway)
{
  int homeVisible = scoreHome % 100;
  int awayVisible = scoreAway % 100;

  if (homeVisible == lastServoHome && awayVisible == lastServoAway)
  {
    return;
  }

  lastServoHome = homeVisible;
  lastServoAway = awayVisible;

  int homeOnes = homeVisible % 10;
  int homeTens = homeVisible / 10;

  int awayOnes = awayVisible % 10;
  int awayTens = awayVisible / 10;

  servoHomeOnes.write(DIGIT_ANGLES[homeOnes]);
  servoHomeTens.write(DIGIT_ANGLES[homeTens]);

  servoAwayOnes.write(DIGIT_ANGLES[awayOnes]);
  servoAwayTens.write(DIGIT_ANGLES[awayTens]);
}