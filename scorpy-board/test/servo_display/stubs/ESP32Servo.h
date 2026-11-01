#pragma once

#include <cassert>
#include <vector>

struct ServoOutput
{
  int frequency = 0;
  int minimum = 0;
  int maximum = 0;
  int angle = -1;
  std::vector<int> writes;
};

extern ServoOutput servoOutputs[40];

class Servo
{
public:
  void setPeriodHertz(int hertz) { frequency = hertz; }

  int attach(int gpio, int minimum, int maximum)
  {
    assert(gpio >= 0 && gpio < 40);
    pin = gpio;
    servoOutputs[pin].frequency = frequency;
    servoOutputs[pin].minimum = minimum;
    servoOutputs[pin].maximum = maximum;
    return 1;
  }

  bool attached() const { return pin >= 0; }

  void write(int angle)
  {
    assert(attached());
    assert(angle >= 0 && angle <= 180);
    servoOutputs[pin].angle = angle;
    servoOutputs[pin].writes.push_back(angle);
  }

private:
  int pin = -1;
  int frequency = 0;
};
