// pairing.cpp

#include <Preferences.h>
#include "pairing.h"

namespace
{
  constexpr const char *PREF_NAMESPACE = "controllers";
  constexpr const char *PREF_HOME_MAC = "home_mac";
  constexpr const char *PREF_AWAY_MAC = "away_mac";

  Preferences prefs;
}

void PairingManager::begin()
{
  homeController = loadController(PREF_HOME_MAC);
  awayController = loadController(PREF_AWAY_MAC);
}

PairMode PairingManager::mode() const
{
  return pairMode;
}

void PairingManager::nextMode()
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
}

void PairingManager::stop()
{
  pairMode = PAIR_OFF;
}

bool PairingManager::isPairing() const
{
  return pairMode != PAIR_OFF;
}

void PairingManager::pairController(const uint8_t mac[MAC_LENGTH])
{
  if (pairMode == PAIR_HOME)
  {
    memcpy(homeController.mac, mac, MAC_LENGTH);
    homeController.isSet = true;
    saveController(PREF_HOME_MAC, mac);

    if (awayController.isSet && macEquals(mac, awayController.mac))
    {
      awayController = {};
      clearController(PREF_AWAY_MAC);
    }
  }
  else if (pairMode == PAIR_AWAY)
  {
    memcpy(awayController.mac, mac, MAC_LENGTH);
    awayController.isSet = true;
    saveController(PREF_AWAY_MAC, mac);

    if (homeController.isSet && macEquals(mac, homeController.mac))
    {
      homeController = {};
      clearController(PREF_HOME_MAC);
    }
  }

  pairMode = PAIR_OFF;
}

bool PairingManager::isHome(const uint8_t mac[MAC_LENGTH]) const
{
  return homeController.isSet && macEquals(mac, homeController.mac);
}

bool PairingManager::isAway(const uint8_t mac[MAC_LENGTH]) const
{
  return awayController.isSet && macEquals(mac, awayController.mac);
}

bool PairingManager::isKnown(const uint8_t mac[MAC_LENGTH]) const
{
  return isHome(mac) || isAway(mac);
}

void PairingManager::clearAll()
{
  prefs.begin(PREF_NAMESPACE);
  prefs.remove(PREF_HOME_MAC);
  prefs.remove(PREF_AWAY_MAC);
  prefs.end();

  homeController = {};
  awayController = {};

  pairMode = PAIR_OFF;
}

PairedController PairingManager::loadController(const char *key)
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

void PairingManager::saveController(const char *key, const uint8_t mac[MAC_LENGTH])
{
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putBytes(key, mac, MAC_LENGTH);
  prefs.end();
}

void PairingManager::clearController(const char *key)
{
  prefs.begin(PREF_NAMESPACE);
  prefs.remove(key);
  prefs.end();
}

bool PairingManager::macEquals(const uint8_t a[MAC_LENGTH], const uint8_t b[MAC_LENGTH]) const
{
  return memcmp(a, b, 6) == 0;
}
