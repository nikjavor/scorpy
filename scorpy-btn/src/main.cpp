// SCORPY-BTN
// main.cpp

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <OneButton.h>

#include <types.h>

bool sendData();
void OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status);
void HandleClick(bool isHome);
void HandleLongPress(bool isHome);

const uint8_t broadcastAddress[] = {0xF4, 0x2D, 0xC9, 0x6C, 0x4C, 0x98}; // MAC address of reciever (scoreboard)

struct_message myData;

const int pinHome = 18;
const int pinAway = 19;

OneButton homeBtn;
OneButton awayBtn;

esp_now_peer_info_t peerInfo;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  homeBtn.setup(pinHome);
  awayBtn.setup(pinAway);

  homeBtn.attachClick([]()
                      { HandleClick(true); });
  homeBtn.attachLongPressStart([]()
                               { HandleLongPress(true); });
  awayBtn.attachClick([]()
                      { HandleClick(false); });
  awayBtn.attachLongPressStart([]()
                               { HandleLongPress(false); });

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSend));

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
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

bool sendData()
{
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK)
  {
    Serial.println("\nSent with success");
  }
  else
  {
    Serial.println("\nError sending the data");
  }
}

void OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void HandleClick(bool isHome)
{
  myData.id = isHome ? 0 : 1;
  myData.value = 1;

  sendData();
}

void HandleLongPress(bool isHome)
{
  myData.id = isHome ? 0 : 1;
  myData.value = -1;

  sendData();
}
