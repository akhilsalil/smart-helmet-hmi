# Smart Helmet HMI

> Collaborators — Akhil Salil, Alby George, Alan Francis, Jerrin Johnson, Noel Gigo

A wearable human-machine interface prototype that allows construction workers to identify robots on site, view their current status, and send operational commands via a helmet-mounted camera and a wrist-mounted touch display.

Built as part of Master project at OTH Regensburg.

## System Components

| Component                            | Status              |
|--------------------------------------|---------------------|
| Mock Service (Flask API + Dashboard) | Working             |
| Wrist Display (ESP32 touch UI)       | Core UI working, ESP-NOW pending |
| Helmet Unit (Nano ESP32 + HuskyLens) | Architecture defined, not built  |
| Demo Robot (2WD chassis)             | Not built                        |

## How It Works

1. Worker wears helmet with HuskyLens camera mounted
2. Worker authenticates on the wrist display with a 4-digit PIN
3. Wrist display fetches the list of robots on site from the mock service and shows them as tappable cards
4. Worker either taps a card directly, or triggers a scan via the SCAN button
5. On scan: HuskyLens on the helmet identifies the robot in view, sends ID to wrist via ESP-NOW
6. Wrist display fetches the selected robot's details from the mock service and shows status + available commands
7. Worker sends a command — wrist POSTs to mock service, service forwards to robot, robot responds, dashboard reflects the change in real time



### Planned: safety bubble

The helmet will passively listen for ESP-NOW beacons broadcast by every robot on site. Each beacon carries the robot's ID and current danger level. When the helmet detects a robot in CAUTION or higher state at close range, it triggers an audible buzzer alarm — warning the worker even when they aren't actively looking at the robot.

A production implementation would use **Ultra-Wideband (UWB)** ranging for true centimetre-accurate proximity detection. UWB chips (e.g. DW3000) measure time-of-flight between paired tags and produce real distance values that don't depend on signal strength, body absorption, or orientation. For this prototype, **ESP-NOW RSSI is used as a makeshift proximity proxy** — coarse but sufficient to demonstrate the concept without additional hardware.


## Architecture (for demo only)

```
              ┌─────────────┐
              │ Phone hotspot│  (2.4 GHz, fixed channel)
              └──────┬───────┘
                     │ Wi-Fi
        ┌────────────┼────────────┬────────────┐
        │            │            │            │
        ▼            ▼            ▼            ▼
  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
  │  Flask   │ │  Wrist   │ │  Helmet  │ │  Robot   │
  │ + dash   │ │ display  │ │ Nano ESP │ │  ESP32   │
  └──────────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘
                    │  ESP-NOW   │            │
                    └────────────┴────────────┘
                       (scan, beacons)
```

Wi-Fi for HTTP traffic (display ↔ Flask, robot ↔ Flask). ESP-NOW for low-latency device-to-device messages (scan trigger, scan result, robot safety beacons).

## Repository Structure

```
smart-helmet-hmi/
├── mockService/       # Flask REST API + web dashboard
├── wristDisplay/      # ESP32 touch display UI (Arduino)
├── helmet/            # Nano ESP32 + HuskyLens code (Arduino)
└── README.md
```

## Hardware

- **Helmet brain:** [Arduino Nano ESP32](https://store.arduino.cc/products/nano-esp32) (ESP32-S3, Wi-Fi + Bluetooth, dual core)
- **Vision:** HuskyLens V1.1 (robot identification via tag recognition)
- **Wrist unit:** [DIYmalls ESP32-3248S035C](https://www.amazon.de/dp/B0C4KSKW96) — 3.5" 480×320 ST7796 capacitive touch display
- **Demo robot:** 2WD chassis + L298N motor driver + ESP32 (Wi-Fi controllable)
- **Buzzer:** active piezo buzzer on the helmet for safety alerts

## Running the Mock Service

See [mockService/README.md](mockService/README.md) for full setup instructions.

## Building the Wrist Display

The wrist display sketch lives in `wristDisplay/`. Required:

- Arduino IDE (2.x recommended)
- ESP32 board package (Espressif Systems)
- Libraries: LVGL **8.3.11** (pinned — do not upgrade to 9.x), TFT_eSPI 2.5.x
- `gt911_lite` touch driver is included in the sketch folder, not as a library
- `config.h` is gitignored — copy `config.example.h` to `config.h` and fill in WiFi credentials, worker PIN, and Flask API URL

**Known limitation:** `lv_conf.h` (LVGL config) and `User_Setup.h` (TFT_eSPI pin config) live inside the Arduino libraries folder, not in this repo. They are required for the sketch to build but are not yet documented or backed up here. To be addressed in a future commit.

## Building the Helmet

Not started. Architecture defined.

## What's Working

- Flask mock API with manager dashboard, 2 robots, command endpoint
- Wrist display: PIN authentication, robot list (fetched from `/robots`), robot detail screen with status colour mapping and dynamic command buttons
- Display → Flask command POST end-to-end (worker taps a command, Flask receives, dashboard updates)

## What's Planned

- ESP-NOW scan workflow (wrist → helmet → wrist)
- HuskyLens integration on helmet
- Safety bubble (robot ESP-NOW beacons → helmet RSSI-triggered alarm)
- Demo robot firmware (motor control via HTTP commands from Flask)
- Robot list refresh / live status updates on wrist display
