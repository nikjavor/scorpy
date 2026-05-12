// SCORPY-BOARD
// main.cpp

#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal.h>
#include <OneButton.h>
#include <Preferences.h>
#include <jled.h>

#include "../../shared/protocol.h"

void displayScore();
void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);
void handleLongPress(int teamId);
void handleLongPressStop(int teamId);
void changeScore(int teamId, int amount);
void resetScores();
PairedController loadPairedController(const char *key);
void savePairedController(const char *key, const uint8_t mac[6]);
bool macEquals(const uint8_t a[6], const uint8_t b[6]);
void clearPairings();

constexpr const char *PREF_NAMESPACE = "controllers";
constexpr const char *PREF_HOME_MAC = "home_mac";
constexpr const char *PREF_AWAY_MAC = "away_mac";

constexpr int PIN_HOME = 25;
constexpr int PIN_AWAY = 26;
constexpr int PIN_PAIR = 27;
constexpr int PIN_LED = 32;

ScorpyMessage message;

LiquidCrystal lcd(4, 18, 19, 21, 22, 23);

Preferences prefs;

OneButton homeBtn;
OneButton awayBtn;
OneButton pairBtn;

PairedController homeController;
PairedController awayController;

bool isHomeLongPressed = false;
bool isAwayLongPressed = false;

int scoreHome = 0;
int scoreAway = 0;

int pairMode = 0;
auto pairHomeBlink = JLed(PIN_LED).Blink(500, 500).Forever();
auto pairAwayBlink = JLed(PIN_LED).Blink(250, 250).Repeat(2).DelayAfter(750).Forever();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  homeBtn.setup(PIN_HOME);
  awayBtn.setup(PIN_AWAY);
  pairBtn.setup(PIN_PAIR);

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

  pairBtn.attachLongPressStart(loopPairMode);

  homeController = loadPairedController(PREF_HOME_MAC);
  awayController = loadPairedController(PREF_AWAY_MAC);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(onDataRecv));

  lcd.begin(16, 2);
  displayScore();
}

void loop()
{
  // put your main code here, to run repeatedly:
  homeBtn.tick();
  awayBtn.tick();
  pairBtn.tick();

  if (pairMode == 0)
  {
    pairHomeBlink.Reset();
    pairAwayBlink.Reset();
  }
  else if (pairMode == 1)
  {
    pairHomeBlink.Update();
  }
  else if (pairMode == 2)
  {
    pairAwayBlink.Update();
  }
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

void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  if (len != sizeof(message))
  {
    Serial.println("Received invalid message size");
    return;
  }

  memcpy(&message, incomingData, sizeof(message));

  if (pairMode == 1 && message.type == EVENT_LONG_PRESS)
  {
    pairController(TEAM_HOME, mac_addr);
    return;
  }
  else if (pairMode == 2 && message.type == EVENT_LONG_PRESS)
  {
    pairController(TEAM_AWAY, mac_addr);
    return;
  }

  if (homeController.isSet && macEquals(mac_addr, homeController.mac))
  {
    if (message.type == EVENT_CLICK)
    {
      changeScore(TEAM_HOME, 1);
    }
    else if (message.type == EVENT_LONG_PRESS)
    {
      changeScore(TEAM_HOME, -1);
    }
  }
  else if (awayController.isSet && macEquals(mac_addr, awayController.mac))
  {
    if (message.type == EVENT_CLICK)
    {
      changeScore(TEAM_AWAY, 1);
    }
    else if (message.type == EVENT_LONG_PRESS)
    {
      changeScore(TEAM_AWAY, -1);
    }
  }
  else
  {
    Serial.println("Unknown controller");
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

void pairController(int teamId, const uint8_t mac[MAC_LENGTH])
{
  if (teamId == TEAM_HOME)
  {
    memcpy(homeController.mac, mac, MAC_LENGTH);
    homeController.isSet = true;
    savePairedController(PREF_HOME_MAC, mac);

    if (awayController.isSet && macEquals(mac, awayController.mac))
    {
      awayController.isSet = false;
      clearPairings(PREF_AWAY_MAC);
    }

    Serial.println("\nPaired new home controller");
  }
  else if (teamId == TEAM_AWAY)
  {
    memcpy(awayController.mac, mac, MAC_LENGTH);
    awayController.isSet = true;
    savePairedController(PREF_AWAY_MAC, mac);

    if (homeController.isSet && macEquals(mac, homeController.mac))
    {
      homeController.isSet = false;
      clearPairings(PREF_HOME_MAC);
    }

    Serial.println("\nPaired new away controller");
  }

  pairMode = 0;
}

PairedController loadPairedController(const char *key)
{
  PairedController controller;
  controller.isSet = false;

  prefs.begin(PREF_NAMESPACE, true); // true = read-only

  size_t len = prefs.getBytesLength(key);

  if (len == MAC_LENGTH)
  {
    prefs.getBytes(key, controller.mac, MAC_LENGTH);
    controller.isSet = true;
  }

  prefs.end();

  return controller;
}

void savePairedController(const char *key, const uint8_t mac[6])
{
  prefs.begin(PREF_NAMESPACE, false); // false = read/write
  prefs.putBytes(key, mac, MAC_LENGTH);
  prefs.end();
}

void clearPairings()
{
  prefs.begin(PREF_NAMESPACE, false);
  prefs.remove(PREF_HOME_MAC);
  prefs.remove(PREF_AWAY_MAC);
  prefs.end();
}

void clearPairings(const char *key)
{
  prefs.begin(PREF_NAMESPACE, false);
  prefs.remove(key);
  prefs.end();

  homeController.isSet = false;
  awayController.isSet = false;
}

bool macEquals(const uint8_t a[6], const uint8_t b[6])
{
  return memcmp(a, b, 6) == 0;
}

void loopPairMode()
{
  pairMode = (pairMode + 1) % 3; // loop 0-1-2-0...

  switch (pairMode)
  {
  case 0:
    Serial.println("\nPairing off");
    break;
  case 1:
    Serial.println("\nPairing home controller");
    break;
  case 2:
    Serial.println("\nPairing away controller");
    break;
  }
}