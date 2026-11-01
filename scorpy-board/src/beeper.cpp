#include <Arduino.h>

#include "beeper.h"
#include "board_pins.h"

namespace
{
  constexpr uint32_t BEEP_DURATION_MS = 200;
  constexpr uint32_t BEEP_GAP_MS = 40;
  constexpr int BEEPER_ON = HIGH; // Powers the active beeper at 3.3 V.
  constexpr int BEEPER_OFF = LOW;

  enum class Phase
  {
    IDLE,
    SOUNDING,
    GAP
  };

  Phase phase = Phase::IDLE;
  uint32_t phaseStartedAt = 0;
  uint32_t pendingBeeps = 0;

  void startBeep()
  {
    digitalWrite(PIN_BEEPER, BEEPER_ON);
    phase = Phase::SOUNDING;
    phaseStartedAt = millis();
  }
}

void beginBeeper()
{
  digitalWrite(PIN_BEEPER, BEEPER_OFF);
  pinMode(PIN_BEEPER, OUTPUT);
  phase = Phase::IDLE;
  pendingBeeps = 0;
}

void beep()
{
  if (phase == Phase::IDLE)
  {
    startBeep();
  }
  else
  {
    ++pendingBeeps;
  }
}

void updateBeeper()
{
  const uint32_t now = millis();

  if (phase == Phase::SOUNDING && now - phaseStartedAt >= BEEP_DURATION_MS)
  {
    digitalWrite(PIN_BEEPER, BEEPER_OFF);
    phase = Phase::GAP;
    phaseStartedAt = now;
  }
  else if (phase == Phase::GAP && now - phaseStartedAt >= BEEP_GAP_MS)
  {
    phase = Phase::IDLE;

    if (pendingBeeps > 0)
    {
      --pendingBeeps;
      startBeep();
    }
  }
}
