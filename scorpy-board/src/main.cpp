// SCORPY-BOARD
// main.cpp

#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal.h>
#include <OneButton.h>
#include <Preferences.h>
#include <jled.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "../../shared/protocol.h"

constexpr int WIRELESS_QUEUE_LENGTH = 10;

struct WirelessEvent
{
  uint8_t mac[MAC_LENGTH];
  ScorpyMessage message;
};

enum PairMode : uint8_t
{
  PAIR_OFF = 0,
  PAIR_HOME = 1,
  PAIR_AWAY = 2
};

void displayScore();
void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);
void handleLongPress(int teamId);
void handleLongPressStop(int teamId);
void changeScore(int teamId, int amount);
void resetScores();
void pairController(int teamId, const uint8_t mac[MAC_LENGTH]);
PairedController loadPairedController(const char *key);
void savePairedController(const char *key, const uint8_t mac[6]);
void clearPairings();
void clearPairedController(const char *key);
bool macEquals(const uint8_t a[6], const uint8_t b[6]);
void loopPairMode();
void updatePairLed();
void handleWirelessEvent(const WirelessEvent &event);
void processWirelessEvents();

constexpr const char *PREF_NAMESPACE = "controllers";
constexpr const char *PREF_HOME_MAC = "home_mac";
constexpr const char *PREF_AWAY_MAC = "away_mac";

constexpr int PIN_HOME = 25;
constexpr int PIN_AWAY = 26;
constexpr int PIN_PAIR = 27;
constexpr int PIN_LED = 32;

QueueHandle_t wirelessQueue;

LiquidCrystal lcd(4, 18, 19, 21, 22, 23);

Preferences prefs;

OneButton homeBtn;
OneButton awayBtn;
OneButton pairBtn;

PairedController homeController = {};
PairedController awayController = {};

bool isHomeLongPressed = false;
bool isAwayLongPressed = false;

int scoreHome = 0;
int scoreAway = 0;

volatile uint32_t droppedQueueEvents = 0;

PairMode pairMode = PAIR_OFF;
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

  wirelessQueue = xQueueCreate(WIRELESS_QUEUE_LENGTH, sizeof(WirelessEvent));
  if (wirelessQueue == nullptr)
  {
    Serial.println("Failed to create wireless queue.");
    ESP.restart();
  }

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

  processWirelessEvents();

  updatePairLed();
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
  if (len != sizeof(ScorpyMessage))
  {
    return;
  }

  WirelessEvent event = {};

  memcpy(event.mac, mac_addr, MAC_LENGTH);
  memcpy(&event.message, incomingData, sizeof(ScorpyMessage));

  bool queued = xQueueSend(wirelessQueue, &event, 0);
  if (queued != pdTRUE)
  {
    droppedQueueEvents++;
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
      awayController = {};
      clearPairedController(PREF_AWAY_MAC);
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
      homeController = {};
      clearPairedController(PREF_HOME_MAC);
    }

    Serial.println("\nPaired new away controller");
  }

  pairMode = PAIR_OFF;
}

PairedController loadPairedController(const char *key)
{
  PairedController controller = {};

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

void clearPairedController(const char *key)
{
  prefs.begin(PREF_NAMESPACE, false);
  prefs.remove(key);
  prefs.end();
}

bool macEquals(const uint8_t a[6], const uint8_t b[6])
{
  return memcmp(a, b, 6) == 0;
}

void loopPairMode()
{
  if (pairMode == PAIR_OFF)
  {
    pairMode = PAIR_HOME;
  }
  else if (pairMode == PAIR_HOME)
  {
    pairMode = PAIR_AWAY;
  }
  else
  {
    pairMode = PAIR_OFF;
  }

  switch (pairMode)
  {
  case PAIR_OFF:
    Serial.println("\nPairing off");
    break;
  case PAIR_HOME:
    Serial.println("\nPairing home controller");
    break;
  case PAIR_AWAY:
    Serial.println("\nPairing away controller");
    break;
  }
}

void updatePairLed()
{
  if (pairMode == PAIR_OFF)
  {
    pairHomeBlink.Reset();
    pairAwayBlink.Reset();
  }
  else if (pairMode == PAIR_HOME)
  {
    pairHomeBlink.Update();
  }
  else if (pairMode == PAIR_AWAY)
  {
    pairAwayBlink.Update();
  }
}

void handleWirelessEvent(const WirelessEvent &event)
{
  const uint8_t *mac_addr = event.mac;
  const ScorpyMessage &message = event.message;

  if (pairMode != PAIR_OFF)
  {
    if (message.type == EVENT_LONG_PRESS)
    {
      if (pairMode == PAIR_HOME)
      {
        pairController(TEAM_HOME, mac_addr);
      }
      else if (pairMode == PAIR_AWAY)
      {
        pairController(TEAM_AWAY, mac_addr);
      }
    }

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

    return;
  }

  if (awayController.isSet && macEquals(mac_addr, awayController.mac))
  {
    if (message.type == EVENT_CLICK)
    {
      changeScore(TEAM_AWAY, 1);
    }
    else if (message.type == EVENT_LONG_PRESS)
    {
      changeScore(TEAM_AWAY, -1);
    }

    return;
  }

  Serial.println("Unknown controller");
}

void processWirelessEvents()
{
  WirelessEvent event;

  while (xQueueReceive(wirelessQueue, &event, 0) == pdTRUE)
  {
    handleWirelessEvent(event);
  }
}