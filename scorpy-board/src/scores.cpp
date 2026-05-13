#include "scores.h"
#include "../../shared/protocol.h"

void ScoreState::change(int teamId, int amount)
{
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
}

void ScoreState::reset()
{
  scoreHome = 0;
  scoreAway = 0;
}

int ScoreState::home() const
{
  return scoreHome;
}

int ScoreState::away() const
{
  return scoreAway;
}
