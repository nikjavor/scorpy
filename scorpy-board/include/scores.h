#include <Arduino.h>

class ScoreState
{
public:
  void change(int teamId, int amount);
  void reset();

  int home() const;
  int away() const;

private:
  int scoreHome = 0;
  int scoreAway = 0;
};