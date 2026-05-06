// SCORPY-BTN
// main.cpp

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>

const uint8_t broadcastAddress[] = {0xF4, 0x2D, 0xC9, 0x6C, 0x4C, 0x98}; // MAC address of reciever (scoreboard)

typedef struct struct_message
{
  int id; // must be unique for each sender board
  int value;
} struct_message;

struct_message myData;

const int pinHome = 18;
const int pinAway = 19;

bool homePressed = false;
bool awayPressed = false;

esp_now_peer_info_t peerInfo;

void OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(pinHome, INPUT_PULLUP);
  pinMode(pinAway, INPUT_PULLUP);

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
  if (digitalRead(pinHome) == LOW)
  {
    if (!homePressed)
    {
      // send
      myData.id = 0;
      myData.value = 1;
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
    homePressed = true;
  }
  else
  {
    homePressed = false;
  }
  
  
  if (digitalRead(pinAway) == LOW)
  {
    if (!awayPressed)
    {
      // send
      myData.id = 1;
      myData.value = 1;
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
    awayPressed = true;
  }
  else
  {
    awayPressed = false;
  }

}
