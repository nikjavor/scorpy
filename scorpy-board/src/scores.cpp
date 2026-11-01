#include "scores.h"
#include "protocol.h"

bool ScoreState::change(int teamId, int amount)
{
  const int previousHome = scoreHome;
  const int previousAway = scoreAway;

  if (teamId == TEAM_HOME)
  {
    scoreHome += amount;

    if (scoreHome < 0)
    {
      scoreHome = 0;
    }
  }
  else if (teamId == TEAM_AWAY)
  {
    scoreAway += amount;

    if (scoreAway < 0)
    {
      scoreAway = 0;
    }
  }

  return scoreHome != previousHome || scoreAway != previousAway;
}

bool ScoreState::reset()
{
  const bool changed = scoreHome != 0 || scoreAway != 0;
  scoreHome = 0;
  scoreAway = 0;
  return changed;
}

int ScoreState::home() const
{
  return scoreHome;
}

int ScoreState::away() const
{
  return scoreAway;
}
