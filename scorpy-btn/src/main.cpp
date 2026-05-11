// SCORPY-BTN
// main.cpp

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <OneButton.h>

#include "../../shared/protocol.h"

bool sendScoreUpdate(int teamId, int value);
void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status);

const uint8_t SCOREBOARD_MAC[] = {0xF4, 0x2D, 0xC9, 0x6C, 0x4C, 0x98}; // MAC address of receiver (scoreboard)

const int PIN_HOME = 18;
const int PIN_AWAY = 19;

OneButton homeBtn;
OneButton awayBtn;

struct_message message;
esp_now_peer_info_t peerInfo;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  homeBtn.setup(PIN_HOME);
  awayBtn.setup(PIN_AWAY);

  homeBtn.attachClick([]()
                      { sendScoreUpdate(TEAM_HOME, 1); });
  homeBtn.attachLongPressStart([]()
                               { sendScoreUpdate(TEAM_HOME, -1); });
  awayBtn.attachClick([]()
                      { sendScoreUpdate(TEAM_AWAY, 1); });
  awayBtn.attachLongPressStart([]()
                               { sendScoreUpdate(TEAM_AWAY, -1); });

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_register_send_cb(esp_now_send_cb_t(onDataSend));

  memcpy(peerInfo.peer_addr, SCOREBOARD_MAC, sizeof(SCOREBOARD_MAC));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    ESP.restart();
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  homeBtn.tick();
  awayBtn.tick();
}

bool sendScoreUpdate(int teamId, int value)
{
  message.id = teamId;
  message.value = value;

  esp_err_t result = esp_now_send(SCOREBOARD_MAC, (uint8_t *)&message, sizeof(message));

  if (result == ESP_OK)
  {
    Serial.println("\nSent with success");
    return true;
  }
  else
  {
    Serial.println("\nError sending the data");
    return false;
  }
}

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
