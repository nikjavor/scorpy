Run from `scorpy-board`:

```sh
c++ -std=c++11 -Wall -Wextra -Werror \
  -Itest/beeper/stubs -Iinclude -I../shared \
  src/beeper.cpp src/scores.cpp test/beeper/test_beeper.cpp \
  -o /tmp/scorpy-beeper-tests
/tmp/scorpy-beeper-tests
```

The tests check silent startup, 80 ms pulses, 40 ms gaps between queued beeps,
clock rollover, and detecting real score changes instead of no-op actions.
