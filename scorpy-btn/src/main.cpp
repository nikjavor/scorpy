// SCORPY-BTN
// main.cpp

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <OneButton.h>

#include "../../shared/protocol.h"

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status);
void handleClick();
void handleLongPress();
bool sendEvent(ClickEventType type);

const uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast MAC

const int BTN_PIN = 10; // D10

const int TEAM_ID = TEAM_HOME;

OneButton btn;

ScorpyMessage message;
esp_now_peer_info_t peerInfo;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  btn.setup(BTN_PIN);

  btn.attachClick(handleClick);
  btn.attachLongPressStart(handleLongPress);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_register_send_cb(esp_now_send_cb_t(onDataSend));

  memcpy(peerInfo.peer_addr, BROADCAST_MAC, sizeof(BROADCAST_MAC));
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
  btn.tick();
}

bool sendEvent(ClickEventType type)
{
  message.type = type;

  esp_err_t result = esp_now_send(BROADCAST_MAC, (uint8_t *)&message, sizeof(message));

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

void handleClick()
{
  sendEvent(EVENT_CLICK);
}

void handleLongPress()
{
  sendEvent(EVENT_LONG_PRESS);
}

void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
