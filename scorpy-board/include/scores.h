#pragma once

#include <Arduino.h>

class ScoreState
{
public:
  // Return true only when a stored score changes.
  bool change(int teamId, int amount);
  bool reset();

  int home() const;
  int away() const;

private:
  int scoreHome = 0;
  int scoreAway = 0;
};
