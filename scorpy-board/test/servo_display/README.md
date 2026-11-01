Run these host tests from the `scorpy-board` directory with a C++ compiler:

```sh
c++ -std=c++11 -Wall -Wextra -Werror \
  -Itest/servo_display/stubs -Iinclude \
  src/servo_display.cpp test/servo_display/test_servo_display.cpp \
  -o /tmp/scorpy-servo-display-tests
/tmp/scorpy-servo-display-tests
```

The tests compile the production servo display code with simulated time and servo
outputs. They check calibration mapping, upward/downward travel, independent
digits, score rollover/reset, movement timing, clock rollover, and new scores
arriving during movement. Physical positioning and ESP-NOW require hardware checks.
