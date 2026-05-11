// SCORPY-BOARD
// main.cpp

#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal.h>
#include <OneButton.h>

#include "../../shared/protocol.h"

void displayScore();
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);
void handleLongPress(int teamId);
void handleLongPressStop(int teamId);
void changeScore(int teamId, int amount);
void resetScores();

const int PIN_HOME = 26;
const int PIN_AWAY = 27;

struct_message message;

LiquidCrystal lcd(4, 18, 19, 21, 22, 23);

OneButton homeBtn;
OneButton awayBtn;

bool isHomeLongPressed = false;
bool isAwayLongPressed = false;

int scoreHome = 0;
int scoreAway = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  homeBtn.setup(PIN_HOME);
  awayBtn.setup(PIN_AWAY);

  homeBtn.attachClick([]()
                      { changeScore(TEAM_HOME, 1); });
  homeBtn.attachLongPressStart([]()
                               { handleLongPress(TEAM_HOME); });
  homeBtn.attachLongPressStop([]()
                              { handleLongPressStop(TEAM_HOME); });
  awayBtn.attachClick([]()
                      { changeScore(TEAM_AWAY, 1); });
  awayBtn.attachLongPressStart([]()
                               { handleLongPress(TEAM_AWAY); });
  awayBtn.attachLongPressStop([]()
                              { handleLongPressStop(TEAM_AWAY); });

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  lcd.begin(16, 2);
  displayScore();
}

void loop()
{
  // put your main code here, to run repeatedly:
  homeBtn.tick();
  awayBtn.tick();
}

void displayScore()
{
  Serial.printf("\nHome: %d \t Away: %d \n", scoreHome, scoreAway);

  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print("HOME   AWAY");

  lcd.setCursor(3, 1);
  lcd.print(scoreHome);

  lcd.setCursor(10, 1);
  lcd.print(scoreAway);
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  if (len != sizeof(message))
  {
    Serial.println("Received invalid message size");
    return;
  }

  memcpy(&message, incomingData, sizeof(message));

  if (message.id == TEAM_HOME || message.id == TEAM_AWAY)
  {
    changeScore(message.id, message.value);
  }
  else
  {
    Serial.println("Received unknown team ID");
  }
}

void handleLongPress(int teamId)
{
  if (teamId == TEAM_HOME)
  {
    isHomeLongPressed = true;
  }
  else if (teamId == TEAM_AWAY)
  {
    isAwayLongPressed = true;
  }

  if (isHomeLongPressed && isAwayLongPressed)
  {
    resetScores();
    return;
  }

  changeScore(teamId, -1);
}

void handleLongPressStop(int teamId)
{
  if (teamId == TEAM_HOME)
  {
    isHomeLongPressed = false;
  }
  else if (teamId == TEAM_AWAY)
  {
    isAwayLongPressed = false;
  }
}

void changeScore(int teamId, int amount)
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

  displayScore();
}

void resetScores()
{
  scoreHome = 0;
  scoreAway = 0;

  displayScore();
}