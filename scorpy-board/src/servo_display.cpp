#include <Arduino.h>
#include <ESP32Servo.h>

#include "board_pins.h"
#include "servo_display.h"

namespace
{
  constexpr uint32_t DEGREE_DELAY_MS = 10;

  struct ServoCalibration
  {
    int up[10];   // Digits 0 through 9.
    int down[10]; // Digits 9 through 0, as in the calibration sketch.
  };

  // Numbered calibration sets from testingproj/src/main copy.cpp.
  constexpr ServoCalibration HOME_TENS = { // #1
      {0, 21, 40, 60, 80, 98, 115, 132, 150, 168},
      {168, 140, 120, 95, 80, 65, 50, 32, 15, 0}};
  constexpr ServoCalibration HOME_ONES = { // #2
      {0, 25, 40, 60, 80, 98, 118, 132, 150, 168},
      {168, 140, 120, 102, 85, 65, 50, 32, 12, 0}};
  constexpr ServoCalibration AWAY_TENS = { // #3
      {0, 21, 40, 55, 75, 93, 110, 127, 145, 163},
      {163, 140, 120, 105, 80, 65, 43, 25, 15, 0}};
  constexpr ServoCalibration AWAY_ONES = { // #4
      {0, 21, 40, 55, 75, 93, 110, 127, 145, 163},
      {163, 140, 120, 105, 80, 65, 43, 25, 15, 0}};

  class DigitServo
  {
  public:
    DigitServo(int pin, const ServoCalibration &calibration)
        : pin(pin), calibration(calibration)
    {
    }

    void begin()
    {
      servo.setPeriodHertz(50);
      servo.attach(pin, 500, 2500);

      if (!servo.attached())
      {
        Serial.printf("Failed to attach servo on GPIO %d\n", pin);
      }

      lastDigit = 0;
      currentAngle = calibration.up[0];
      targetAngle = currentAngle;
      lastStepAt = millis();
      servo.write(currentAngle);
    }

    void update(int digit, uint32_t now)
    {
      if (digit != lastDigit)
      {
        const int upAngle = calibration.up[digit];
        const int downAngle = calibration.down[9 - digit];

        // A new score can arrive before the previous move finishes. Choose
        // the table that matches the approach from the current commanded angle.
        if (currentAngle < downAngle)
        {
          targetAngle = upAngle;
        }
        else if (currentAngle > upAngle)
        {
          targetAngle = downAngle;
        }
        else
        {
          targetAngle = digit > lastDigit ? upAngle : downAngle;
        }

        lastDigit = digit;
      }

      if (currentAngle == targetAngle)
      {
        lastStepAt = now;
        return;
      }

      if (now - lastStepAt < DEGREE_DELAY_MS)
      {
        return;
      }

      lastStepAt = now;
      currentAngle += currentAngle < targetAngle ? 1 : -1;
      servo.write(currentAngle);
    }

  private:
    Servo servo;
    const int pin;
    const ServoCalibration &calibration;
    int lastDigit = 0;
    int currentAngle = 0;
    int targetAngle = 0;
    uint32_t lastStepAt = 0;
  };

  DigitServo servoHomeOnes(PIN_SERVO_HO, HOME_ONES);
  DigitServo servoHomeTens(PIN_SERVO_HT, HOME_TENS);
  DigitServo servoAwayOnes(PIN_SERVO_AO, AWAY_ONES);
  DigitServo servoAwayTens(PIN_SERVO_AT, AWAY_TENS);

  int lastServoHome = -1;
  int lastServoAway = -1;
}

void beginServos()
{
  servoHomeOnes.begin();
  servoHomeTens.begin();
  servoAwayOnes.begin();
  servoAwayTens.begin();

  lastServoHome = -1;
  lastServoAway = -1;
}

void updateServos(int scoreHome, int scoreAway)
{
  const int homeVisible = scoreHome < 0 ? 0 : scoreHome % 100;
  const int awayVisible = scoreAway < 0 ? 0 : scoreAway % 100;

  if (homeVisible != lastServoHome || awayVisible != lastServoAway)
  {
    lastServoHome = homeVisible;
    lastServoAway = awayVisible;
    Serial.printf("\nHome: %d \t Away: %d \n", homeVisible, awayVisible);
  }

  const uint32_t now = millis();
  servoHomeOnes.update(homeVisible % 10, now);
  servoHomeTens.update(homeVisible / 10, now);
  servoAwayOnes.update(awayVisible % 10, now);
  servoAwayTens.update(awayVisible / 10, now);
}
