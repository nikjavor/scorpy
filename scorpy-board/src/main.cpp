// SCORPY-BOARD
// main.cpp

#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>

typedef struct struct_message
{
  int id;
  int value;
} struct_message;

struct_message myData;

int score_home = 0;
int score_guest = 0;

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  // char macStr[18];
  // Serial.print("Packet received from: ");
  // snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
  //          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  // Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  if (myData.id == 0)
  {
    score_home += myData.value;
  }
  else if (myData.id == 1)
  {
    score_guest += myData.value;
  }
  Serial.printf("\nHome: %d \t Guest: %d \n", score_home, score_guest);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  Serial.printf("\nHome: %d \t Guest: %d \n", score_home, score_guest);

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10);
}
