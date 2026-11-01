#pragma once

#include <cstddef>
#include <cstdint>

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int OUTPUT = 3;

uint32_t millis();
void pinMode(int pin, int mode);
void digitalWrite(int pin, int level);
