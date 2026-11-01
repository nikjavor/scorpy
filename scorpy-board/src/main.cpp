// SCORPY-BOARD
// main.cpp

#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <OneButton.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "protocol.h"

#include "board_pins.h"
#include "beeper.h"
#include "servo_display.h"
#include "pairing.h"
#include "scores.h"

constexpr int WIRELESS_QUEUE_LENGTH = 10;

struct WirelessEvent
{
  uint8_t mac[MAC_LENGTH];
  ScorpyMessage message;
};

QueueHandle_t wirelessQueue;

OneButton homeBtn;
OneButton awayBtn;
OneButton pairBtn;

PairingManager pairing;

ScoreState scores;

bool isHomeLongPressed = false;
bool isAwayLongPressed = false;

volatile uint32_t droppedQueueEvents = 0;

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
void changeScore(int teamId, int amount);
void resetScores();
void handleLongPress(int teamId);
void handleLongPressStop(int teamId);
void updatePairLed();
void handleWirelessEvent(const WirelessEvent &event);
void processWirelessEvents();

void setup()
{
  Serial.begin(115200);

  homeBtn.setup(PIN_HOME);
  awayBtn.setup(PIN_AWAY);
  pairBtn.setup(PIN_PAIR);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  beginBeeper();

  pairing.begin();

  beginServos();

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

  pairBtn.attachLongPressStart([]()
                               { pairing.nextMode(); });

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

  if (esp_now_register_recv_cb(onDataRecv) != ESP_OK)
  {
    Serial.println("Error registering ESP-NOW receive callback");
    ESP.restart();
  }
}

void loop()
{
  homeBtn.tick();
  awayBtn.tick();
  pairBtn.tick();

  processWirelessEvents();

  updateBeeper();
  updatePairLed();
  updateServos(scores.home(), scores.away());
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  if (len != sizeof(ScorpyMessage))
  {
    return;
  }

  WirelessEvent event = {};

  memcpy(event.mac, mac, MAC_LENGTH);
  memcpy(&event.message, incomingData, sizeof(ScorpyMessage));

  bool queued = xQueueSend(wirelessQueue, &event, 0);
  if (queued != pdTRUE)
  {
    droppedQueueEvents += 1;
  }
}

void changeScore(int teamId, int amount)
{
  if (scores.change(teamId, amount))
  {
    beep();
  }
}

void resetScores()
{
  if (scores.reset())
  {
    beep();
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

void updatePairLed()
{
  static PairMode previousMode = PAIR_OFF;
  static uint32_t modeStartedAt = 0;

  const PairMode mode = pairing.mode();
  const uint32_t now = millis();

  if (mode != previousMode)
  {
    previousMode = mode;
    modeStartedAt = now;
  }

  if (mode == PAIR_OFF)
  {
    digitalWrite(PIN_LED, HIGH);
    return;
  }

  // Digital output keeps the status LED off the servos' PWM channels/timers.
  const uint32_t interval = mode == PAIR_HOME ? 500 : 200;
  const bool ledOn = ((now - modeStartedAt) / interval) % 2 == 0;
  digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
}

void handleWirelessEvent(const WirelessEvent &event)
{
  const uint8_t *mac_addr = event.mac;
  const ScorpyMessage &message = event.message;

  Serial.printf(
      "MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
      mac_addr[0],
      mac_addr[1],
      mac_addr[2],
      mac_addr[3],
      mac_addr[4],
      mac_addr[5]);

  if (pairing.isPairing())
  {
    if (message.type == EVENT_LONG_PRESS)
    {
      pairing.pairController(mac_addr);
    }

    return;
  }

  if (pairing.isHome(mac_addr))
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

  if (pairing.isAway(mac_addr))
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
