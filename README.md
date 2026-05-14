# Scorpy

Scorpy is a player-controlled physical scoreboard.

Each team has one small wireless button. When a player presses their button, the scoreboard updates that team’s score.

The goal is simple scoring without a phone, app, or referee remote.

## How It Works

```text
[ Home Button ]  ─┐
                  ├─ ESP-NOW ─>  [ Scoreboard ]
[ Away Button ]  ─┘
```

Scorpy has three devices:

- Home button — used by the home team
- Away button — used by the away team
- Scoreboard — receives button events and controls the score display

The scoreboard uses servo-driven mechanical 7-segment displays instead of a normal screen.

## Controls

### Wearable Buttons

| Action     | Result                            |
| ---------- | --------------------------------- |
| Click      | Add 1 point                       |
| Long press | Subtract 1 point / pairing action |

### Scoreboard

| Action                 | Result            |
| ---------------------- | ----------------- |
| Home button            | Change Home score |
| Away button            | Change Away score |
| Pair button            | Pair a controller |
| Home + Away long press | Reset scores      |

## Pairing

Buttons are paired to the scoreboard by MAC address.

To pair a controller:

1. Long press the Pair button on the scoreboard.
2. Choose Home or Away pairing mode.
3. Long press the wearable button.
4. The scoreboard saves that controller.

This makes it possible to replace a broken button without changing the code.

## Hardware

### Button Controller

- ESP32-based board
- One tactile button
- Battery-powered enclosure
- ESP-NOW wireless communication

### Scoreboard

- ESP32 development board
- Local control buttons
- Pairing LED
- Four servos
- Mechanical rack-driven 7-segment display

## Project Structure

```text
scorpy/
  scorpy-board/ # Scoreboard firmware
  scorpy-btn/ # Wearable button firmware
  shared/ # Shared protocol/types
```

## Goal

To build a simple, physical, player-controlled scoreboard for casual sports games, starting with beach volleyball.
