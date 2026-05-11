// SCORPY-BOARD
// main.cpp

#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal.h>
#include <OneButton.h>

#include <types.h>

void displayScore();
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);
void HandleClick(bool isHome);
void HandleLongPress(bool isHome);
void HandleLongPressStop(bool isHome);

struct_message myData;

LiquidCrystal lcd(4, 18, 19, 21, 22, 23);

int pinHome = 26;
int pinGuest = 27;

OneButton homeBtn;
OneButton guestBtn;

bool isHomeLongPressed = false;
bool isGuestLongPressed = false;

int score_home = 0;
int score_guest = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  homeBtn.setup(pinHome);
  guestBtn.setup(pinGuest);

  homeBtn.attachClick([]()
                      { HandleClick(true); });
  homeBtn.attachLongPressStart([]()
                               { HandleLongPress(true); });
  homeBtn.attachLongPressStop([]()
                              { HandleLongPressStop(true); });
  guestBtn.attachClick([]()
                       { HandleClick(false); });
  guestBtn.attachLongPressStart([]()
                                { HandleLongPress(false); });
  guestBtn.attachLongPressStop([]()
                               { HandleLongPressStop(true); });

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  lcd.begin(16, 2);
  displayScore();
}

void loop()
{
  // put your main code here, to run repeatedly:
  homeBtn.tick();
  guestBtn.tick();
  delay(10);
}

void displayScore()
{
  Serial.printf("\nHome: %d \t Guest: %d \n", score_home, score_guest);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("HOME   GUEST");
  lcd.setCursor(3, 1);
  lcd.print(score_home);
  lcd.setCursor(11, 1);
  lcd.print(score_guest);
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  memcpy(&myData, incomingData, sizeof(myData));

  if (myData.id == 0)
  {
    score_home += myData.value;
    if (score_home < 0)
      score_home = 0;
  }
  else if (myData.id == 1)
  {
    score_guest += myData.value;
    if (score_guest < 0)
      score_guest = 0;
  }

  displayScore();
}

void HandleClick(bool isHome)
{
  if (isHome)
  {
    score_home += 1;
  }
  else
  {
    score_guest += 1;
  }

  displayScore();
}

void HandleLongPress(bool isHome)
{
  if (isHome)
  {
    isHomeLongPressed = true;
    if (score_home > 0)
      score_home -= 1;
  }
  else
  {
    isGuestLongPressed = true;
    if (score_guest > 0)
      score_guest -= 1;
  }

  if (isHomeLongPressed && isGuestLongPressed)
  {
    score_home = 0;
    score_guest = 0;
  }

  displayScore();
}

void HandleLongPressStop(bool isHome)
{
  if (isHome)
  {
    isHomeLongPressed = false;
  }
  else
  {
    isGuestLongPressed = false;
  }
}