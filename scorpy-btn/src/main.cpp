// SCORPY-BTN
// main.cpp

#include <Arduino.h>

#include <esp_now.h>
#include <esp_sleep.h>
#include <WiFi.h>

#include "../../shared/protocol.h"

constexpr uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast MAC

constexpr int BTN_PIN = D1; // D1

constexpr uint32_t LONG_PRESS_MS = 800;
constexpr uint32_t DEBOUNCE_MS = 30;

ScorpyMessage message;
esp_now_peer_info_t peerInfo;

void initEspNow();
void goToSleep();
bool sendEvent(ClickEventType type);
ClickEventType detectPressType();

void setup()
{
  // put your setup code here, to run on wake

  pinMode(BTN_PIN, INPUT_PULLUP);

  delay(DEBOUNCE_MS);

  if (digitalRead(BTN_PIN) == HIGH)
  {
    goToSleep();
  }

  ClickEventType eventType = detectPressType();

  initEspNow();

  sendEvent(eventType);
  delay(100);

  // Wait until button is released so it does not instantly wake again.
  while (digitalRead(BTN_PIN) == LOW)
  {
    delay(10);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  goToSleep();
}

void loop()
{
}

void goToSleep()
{
  pinMode(BTN_PIN, INPUT_PULLUP);

  uint64_t wakeMask = 1ULL << BTN_PIN;

  esp_deep_sleep_enable_gpio_wakeup(
      wakeMask,
      ESP_GPIO_WAKEUP_GPIO_LOW);

  Serial.flush();
  esp_deep_sleep_start();
}

ClickEventType detectPressType()
{
  uint32_t pressStart = millis();

  while (digitalRead(BTN_PIN) == LOW)
  {
    if (millis() - pressStart >= LONG_PRESS_MS)
    {
      return EVENT_LONG_PRESS;
    }

    delay(5);
  }

  return EVENT_CLICK;
}

void initEspNow()
{
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, BROADCAST_MAC, sizeof(BROADCAST_MAC));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    ESP.restart();
  }
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
