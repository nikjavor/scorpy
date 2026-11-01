#include <Arduino.h>
#include <ESP32Servo.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "board_pins.h"
#include "servo_display.h"

TestSerial Serial;
ServoOutput servoOutputs[40];
uint32_t testTime = 0;

uint32_t millis() { return testTime; }

namespace
{
  const int servoPins[] = {PIN_SERVO_HO, PIN_SERVO_HT, PIN_SERVO_AO, PIN_SERVO_AT};

  void reset(uint32_t startTime = 0)
  {
    testTime = startTime;
    for (int pin : servoPins)
    {
      servoOutputs[pin] = ServoOutput{};
    }
    beginServos();
  }

  void advance(int home, int away, int ticks = 180)
  {
    for (int tick = 0; tick < ticks; ++tick)
    {
      testTime += 10;
      updateServos(home, away);
    }
  }

  void expectAngles(int homeOnes, int homeTens, int awayOnes, int awayTens)
  {
    assert(servoOutputs[PIN_SERVO_HO].angle == homeOnes);
    assert(servoOutputs[PIN_SERVO_HT].angle == homeTens);
    assert(servoOutputs[PIN_SERVO_AO].angle == awayOnes);
    assert(servoOutputs[PIN_SERVO_AT].angle == awayTens);
  }

  void testCalibrationAndIndependentDigits()
  {
    reset();
    expectAngles(0, 0, 0, 0);
    for (int pin : servoPins)
    {
      assert(servoOutputs[pin].frequency == 50);
      assert(servoOutputs[pin].minimum == 500);
      assert(servoOutputs[pin].maximum == 2500);
    }

    advance(66, 66);
    expectAngles(118, 115, 110, 110);
    advance(33, 33);
    expectAngles(50, 50, 43, 43);

    const auto awayWrites = servoOutputs[PIN_SERVO_AO].writes.size();
    advance(43, 33);
    expectAngles(50, 80, 43, 43);
    assert(servoOutputs[PIN_SERVO_AO].writes.size() == awayWrites);

    advance(0, 0);
    expectAngles(0, 0, 0, 0);
  }

  void testRolloverAndInputBounds()
  {
    reset();
    advance(9, 0);
    expectAngles(168, 0, 0, 0);
    advance(10, 0);
    expectAngles(0, 21, 0, 0);
    advance(9, 0);
    expectAngles(168, 0, 0, 0);
    advance(99, 99);
    expectAngles(168, 168, 163, 163);
    advance(100, 100);
    expectAngles(0, 0, 0, 0);
    advance(-1, -99);
    expectAngles(0, 0, 0, 0);
  }

  void testNonBlockingStepsAndClockRollover()
  {
    reset(UINT32_MAX - 5);
    updateServos(11, 11);
    expectAngles(0, 0, 0, 0);
    testTime += 9;
    updateServos(11, 11);
    expectAngles(0, 0, 0, 0);
    testTime += 1;
    updateServos(11, 11);
    expectAngles(1, 1, 1, 1);

    // A late loop iteration advances once, without blocking to catch up.
    testTime += 500;
    updateServos(11, 11);
    expectAngles(2, 2, 2, 2);
    advance(11, 11);
    expectAngles(25, 21, 21, 21);

    for (int pin : servoPins)
    {
      const auto &writes = servoOutputs[pin].writes;
      for (size_t index = 1; index < writes.size(); ++index)
      {
        assert(std::abs(writes[index] - writes[index - 1]) == 1);
      }
    }
  }

  void testRetargetDuringMovement()
  {
    reset();
    advance(9, 0, 10);
    expectAngles(10, 0, 0, 0);
    advance(3, 0);
    // Still approaching digit 3 from below, despite the requested score falling.
    expectAngles(60, 0, 0, 0);

    advance(9, 0);
    advance(0, 0, 10);
    expectAngles(158, 0, 0, 0);
    advance(6, 0);
    // Still approaching digit 6 from above, despite the requested score rising.
    expectAngles(102, 0, 0, 0);
  }
}

int main()
{
  testCalibrationAndIndependentDigits();
  testRolloverAndInputBounds();
  testNonBlockingStepsAndClockRollover();
  testRetargetDuringMovement();
  std::puts("All servo display tests passed.");
}
