#pragma once

#include <cstdint>

uint32_t millis();

struct TestSerial
{
  template <typename... Args>
  void printf(const char *, Args...) {}
};

extern TestSerial Serial;
