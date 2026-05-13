// pairing.h

#pragma once

#include <Arduino.h>
#include "../../shared/protocol.h"

enum PairMode : uint8_t
{
  PAIR_OFF = 0,
  PAIR_HOME = 1,
  PAIR_AWAY = 2
};

class PairingManager
{
public:
  void begin();

  PairMode mode() const;
  void nextMode();
  void stop();

  bool isPairing() const;

  void pairController(const uint8_t mac[MAC_LENGTH]);

  bool isHome(const uint8_t mac[MAC_LENGTH]) const;
  bool isAway(const uint8_t mac[MAC_LENGTH]) const;
  bool isKnown(const uint8_t mac[MAC_LENGTH]) const;

  void clearAll();

private:
  PairMode pairMode = PAIR_OFF;

  PairedController homeController = {};
  PairedController awayController = {};

  PairedController loadController(const char *key);
  void saveController(const char *key, const uint8_t mac[MAC_LENGTH]);
  void clearController(const char *key);

  bool macEquals(const uint8_t a[MAC_LENGTH], const uint8_t b[MAC_LENGTH]) const;
};