#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <vector>

#include "beeper.h"
#include "board_pins.h"
#include "protocol.h"
#include "scores.h"

uint32_t testTime = 0;
int beeperLevel = -1;
int beeperMode = -1;
std::vector<int> writes;

uint32_t millis() { return testTime; }

void pinMode(int pin, int mode)
{
  assert(pin == PIN_BEEPER);
  beeperMode = mode;
}

void digitalWrite(int pin, int level)
{
  assert(pin == PIN_BEEPER);
  beeperLevel = level;
  writes.push_back(level);
}

namespace
{
  void reset(uint32_t startTime = 0)
  {
    testTime = startTime;
    writes.clear();
    beginBeeper();
    assert(beeperLevel == LOW);
    assert(beeperMode == OUTPUT);
  }

  void advance(uint32_t milliseconds)
  {
    testTime += milliseconds;
    updateBeeper();
  }

  void testSilentStartupAndTimedBeep()
  {
    reset();
    advance(1000);
    assert(writes.size() == 1);

    beep();
    assert(testTime == 1000); // Starting a beep never waits.
    assert(beeperLevel == HIGH);
    advance(79);
    assert(beeperLevel == HIGH);
    advance(1);
    assert(beeperLevel == LOW);
    advance(1000);
    assert((writes == std::vector<int>{LOW, HIGH, LOW}));
  }

  void testBurstProducesSeparateBeeps()
  {
    reset();
    beep();
    beep();
    beep();
    advance(80);
    assert(beeperLevel == LOW);
    advance(39);
    assert(beeperLevel == LOW);
    advance(1);
    assert(beeperLevel == HIGH);
    advance(80);
    advance(40);
    assert(beeperLevel == HIGH);
    advance(80);
    advance(40);
    advance(1000);
    assert((writes == std::vector<int>{LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW}));

    // A change during the gap also gets a full, separate pulse.
    reset();
    beep();
    advance(80);
    beep();
    advance(40);
    advance(80);
    assert((writes == std::vector<int>{LOW, HIGH, LOW, HIGH, LOW}));
  }

  void testClockRolloverAndRestart()
  {
    reset(UINT32_MAX - 39);
    beep();
    advance(79);
    assert(beeperLevel == HIGH);
    advance(1);
    assert(beeperLevel == LOW);
    beep();
    advance(40);
    assert(beeperLevel == HIGH);

    beginBeeper();
    advance(1000);
    assert(beeperLevel == LOW);
  }

  void testOnlyActualScoreChangesAreReported()
  {
    ScoreState scores;
    assert(!scores.reset());
    assert(!scores.change(TEAM_HOME, -1));
    assert(!scores.change(TEAM_AWAY, -1));
    assert(!scores.change(99, 1));
    assert(!scores.change(TEAM_HOME, 0));
    assert(scores.change(TEAM_HOME, 1));
    assert(scores.change(TEAM_AWAY, 2));
    assert(scores.change(TEAM_HOME, -1));
    assert(!scores.change(TEAM_HOME, -1));
    assert(scores.home() == 0 && scores.away() == 2);
    assert(scores.reset());
    assert(scores.home() == 0 && scores.away() == 0);
    assert(!scores.reset());
  }
}

int main()
{
  testSilentStartupAndTimedBeep();
  testBurstProducesSeparateBeeps();
  testClockRolloverAndRestart();
  testOnlyActualScoreChangesAreReported();
  std::puts("All beeper and score change tests passed.");
}
