// shared/protocol.h
#pragma once

enum ClickEventType : uint8_t
{
  EVENT_CLICK = 1,
  EVENT_LONG_PRESS = 2,
  EVENT_DOUBLE_CLICK = 3
};

typedef struct ScorpyMessage
{
  ClickEventType type;
} ScorpyMessage;

typedef struct PairedController
{
  uint8_t mac[6];
  bool isSet;
} PairedController;

constexpr int TEAM_HOME = 0;
constexpr int TEAM_AWAY = 1;

constexpr size_t MAC_LENGTH = 6;
