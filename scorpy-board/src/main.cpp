// SCORPY-BOARD
// main.cpp

#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal.h>
#include <OneButton.h>
#include <jled.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "../../shared/protocol.h"

#include "board_pins.h"
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

JLed pairHomeBlink = JLed(PIN_LED).Blink(500, 500).Forever();
JLed pairAwayBlink = JLed(PIN_LED).Blink(200, 200).Forever();
JLed pairLedOn = JLed(PIN_LED).On().Forever();

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);
void handleLongPress(int teamId);
void handleLongPressStop(int teamId);
void updatePairLed();
void handleWirelessEvent(const WirelessEvent &event);
void processWirelessEvents();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  homeBtn.setup(PIN_HOME);
  awayBtn.setup(PIN_AWAY);
  pairBtn.setup(PIN_PAIR);

  pairing.begin();

  beginServos();

  homeBtn.attachClick([]()
                      { scores.change(TEAM_HOME, 1); });
  homeBtn.attachLongPressStart([]()
                               { handleLongPress(TEAM_HOME); });
  homeBtn.attachLongPressStop([]()
                              { handleLongPressStop(TEAM_HOME); });

  awayBtn.attachClick([]()
                      { scores.change(TEAM_AWAY, 1); });
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

  esp_now_register_recv_cb(onDataRecv);
}

void loop()
{
  // put your main code here, to run repeatedly:
  homeBtn.tick();
  awayBtn.tick();
  pairBtn.tick();

  processWirelessEvents();

  updatePairLed();
  updateServos(scores.home(), scores.away());
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
  if (len != sizeof(ScorpyMessage))
  {
    return;
  }

  WirelessEvent event = {};

  memcpy(event.mac, info->src_addr, MAC_LENGTH);
  memcpy(&event.message, incomingData, sizeof(ScorpyMessage));

  bool queued = xQueueSend(wirelessQueue, &event, 0);
  if (queued != pdTRUE)
  {
    droppedQueueEvents += 1;
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
    scores.reset();
    return;
  }

  scores.change(teamId, -1);
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
  if (pairing.mode() == PAIR_OFF)
  {
    pairLedOn.Update();

    pairHomeBlink.Reset();
    pairAwayBlink.Reset();
  }
  else if (pairing.mode() == PAIR_HOME)
  {
    pairHomeBlink.Update();

    pairAwayBlink.Reset();
    pairLedOn.Reset();
  }
  else if (pairing.mode() == PAIR_AWAY)
  {
    pairAwayBlink.Update();

    pairHomeBlink.Reset();
    pairLedOn.Reset();
  }
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
      scores.change(TEAM_HOME, 1);
    }
    else if (message.type == EVENT_LONG_PRESS)
    {
      scores.change(TEAM_HOME, -1);
    }

    return;
  }

  if (pairing.isAway(mac_addr))
  {
    if (message.type == EVENT_CLICK)
    {
      scores.change(TEAM_AWAY, 1);
    }
    else if (message.type == EVENT_LONG_PRESS)
    {
      scores.change(TEAM_AWAY, -1);
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