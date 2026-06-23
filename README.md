# Smart Helmet HMI

> Collaborators - Akhil Salil, Alby George, Alan Francis, Jerrin Johnson, Noel Gijo

A wearable human-machine interface prototype that allows construction workers to identify robots on site, view their current status, and send operational commands via a helmet-mounted camera and a wrist-mounted touch display.

Built as part of Master project at OTH Regensburg.

## System Components

| Component                            | Status              |
|--------------------------------------|---------------------|
| Mock Service (Flask API + Dashboard) | Working             |
| Wrist Display (ESP32 touch UI)       | Working - all core flows + roles + haptic alert |
| Helmet Unit (Nano ESP32 + HuskyLens) | Working - ESP-NOW + real HuskyLens scan + safety bubble |
| BLE Scanner (LOLIN32 on helmet)      | Working - relays robot BLE beacons to helmet via ESP-NOW |
| Demo Robot (2WD chassis)             | In development      |
| Dobot Magician Go (lab robot)        | Integrated via DobotLab file-polling bridge |

## How It Works

1. Worker wears helmet with HuskyLens camera and a BLE scanner board mounted
2. Worker authenticates on the wrist display with a 4-digit PIN (OPERATOR or VIEWER role)
3. Wrist display fetches the list of robots on site from the mock service and shows them as tappable cards
4. Worker either taps a card directly, or triggers a scan via the SCAN button
5. On scan: HuskyLens on the helmet identifies the robot in view via Tag Recognition, sends ID to wrist via ESP-NOW
6. Wrist display fetches the selected robot's details from the mock service and shows status + available commands
7. Worker sends a command - wrist POSTs to mock service, service forwards to robot, robot responds, dashboard reflects the change in real time

### Safety Bubble

The helmet monitors proximity to dangerous robots and alerts the worker via the wrist display's vibration motor - providing a haptic channel that works under ear protection and without the worker needing to look at the display.

**How proximity is measured:** a dedicated BLE scanner board (WEMOS LOLIN32) mounted on the helmet passively listens for BLE advertisements broadcast by the robot's ESP32. Each advertisement carries `robotId` and `dangerLevel` in a custom manufacturer data payload (`0xFFFF` company ID, version byte, robotId, dangerLevel - 5 bytes total). The scanner reads RSSI per advertisement (signal strength = proximity proxy), packages `robotId + dangerLevel + rssi` into an ESP-NOW packet, and forwards it to the helmet's Nano over ESP-NOW. This keeps BLE off the Nano's radio, which is already occupied by WiFi + ESP-NOW for the scan workflow.

The helmet Nano runs the bubble state machine:
- EMA smoothing of RSSI (α=0.3) to reduce noise
- Hysteresis: enter alarm at ≥ −70 dBm, exit at ≤ −78 dBm
- Danger threshold: dangerLevel ≥ 1 (CAUTION) required to trigger
- 3s beacon timeout: robot considered gone if no beacon received
- 2s minimum alarm hold: prevents rapid on/off flickering

When alarm state changes, the helmet sends an ESP-NOW `AlarmStateMsg` (4 bytes, msgType 30) to the wrist. The wrist drives a vibration motor (GPIO 21) with intensity and pattern scaled to danger level:

- **Level 1 (CAUTION):** single 2s buzz
- **Level 2 (SENSITIVE):** 500ms on/off pulsing until cleared or 10s cutoff
- **Level 3 (HIGH):** continuous until cleared or 10s cutoff

A production implementation would use **Ultra-Wideband (UWB)** ranging for true centimetre-accurate proximity detection. For this prototype, BLE RSSI is used as a coarse but sufficient proximity proxy to demonstrate the concept.

## Architecture (for demo only)

```
              ┌─────────────┐
              │ Phone hotspot│  (2.4 GHz, channel auto-detected at runtime)
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
                    (scan, alarm-state messages)

                                 ▲  ESP-NOW (beacon relay)
                                 │
                          ┌──────────────┐
                          │ BLE Scanner  │  ← BLE advertisements
                          │  (LOLIN32)   │ ←←←←←←←←←←←←←←←←←←←←←┐
                          └──────────────┘                          │
                                                               ┌──────────┐
                                                               │  Robot   │
                                                               │  ESP32   │
                                                               │(BLE adv) │
                                                               └──────────┘
```

Wi-Fi for HTTP traffic (display ↔ Flask, robot ↔ Flask). ESP-NOW for low-latency device-to-device messages (scan trigger, scan result, helmet → wrist alarm-state, BLE scanner → helmet beacon relay).

## Repository Structure

```
smart-helmet-hmi/
├── mockService/       # Flask REST API + web dashboard
├── wristDisplay/      # ESP32 touch display UI
├── helmet/            # Nano ESP32 + HuskyLens code (Arduino)
├── bleScanner/        # LOLIN32 BLE scanner sketch
└── README.md
```

## Hardware

- **Helmet brain:** [Arduino Nano ESP32](https://store.arduino.cc/products/nano-esp32) (ESP32-S3, Wi-Fi + Bluetooth, dual core)
- **Vision:** HuskyLens V1.1 (robot identification via Tag Recognition mode)
- **BLE scanner:** WEMOS LOLIN32 (ESP32-WROOM-32) - dedicated BLE scanner board mounted on helmet, relays robot proximity beacons to Nano over ESP-NOW. Keeps BLE off the Nano's already-busy radio.
- **Wrist unit:** [DIYmalls ESP32-3248S035C](https://www.amazon.de/dp/B0C4KSKW96) - 3.5" 480×320 ST7796 capacitive touch display
- **Wrist haptic:** vibration motor module (MOSFET-driven, PWM-capable) on GPIO 21 (CN1 connector, 3.3V)
- **Demo robot:** 2WD chassis + L298N motor driver + ESP32 (Wi-Fi controllable, BLE advertiser for safety bubble - in development)
- **Lab robot:** Dobot Magician Go + Magician Lite (mobile platform with mecanum wheels + arm) - OTH Regensburg lab hardware, integrated via DobotLab Python bridge

## Running the Mock Service

See [mockService/README.md](mockService/README.md) for full setup instructions.

## Building the Wrist Display

The wrist display sketch lives in `wristDisplay/`. Required:

- Arduino IDE (2.x recommended)
- ESP32 board package (Espressif Systems, mainline 3.x core)
- Libraries: LVGL **8.3.11** (pinned - do not upgrade to 9.x), TFT_eSPI 2.5.x
- `gt911_lite` touch driver is included in the sketch folder, not as a library
- `config.h` is gitignored - copy `config.example.h` to `config.h` and fill in WiFi credentials, worker PINs, Flask API URL, and helmet MAC address

**Known limitation:** `lv_conf.h` (LVGL config) and `User_Setup.h` (TFT_eSPI pin config) live inside the Arduino libraries folder, not in this repo. They are required for the sketch to build but are not yet documented or backed up here. To be addressed in a future commit.

**WiFi sleep must be disabled** (`WiFi.setSleep(false)` after connect) - without it, ESP-NOW send reliability drops to ~30%. Already applied in `wifi_manager.cpp`.

## Building the Helmet

The helmet sketch lives in `helmet/`. Required:

- Arduino IDE (2.x recommended)
- **ESP32 board package: Arduino's frozen "Arduino Nano ESP32" core (2.0.18-arduino.5)**, not Espressif's mainline core. Arduino's IDE installs this automatically when "Arduino Nano ESP32" is selected as the board.
- Libraries: HUSKYLENS by DFRobot (via Arduino Library Manager)
- `config.h` is gitignored - copy `config.example.h` to `config.h` and fill in WiFi credentials and the wrist's MAC address

**Note on ESP-NOW core asymmetry:** the helmet runs Arduino's frozen 2.0.18-arduino.5 core, while the wrist runs Espressif's mainline 3.x core. ESP-NOW callback signatures differ between the two - see comments at the top of each board's `espnow_manager.cpp`. Do not copy callbacks between boards without checking.

## Building the BLE Scanner

The BLE scanner sketch lives in `bleScanner/`. Required:

- Arduino IDE (2.x recommended)
- ESP32 board package (Espressif Systems, mainline - select "WEMOS LOLIN32" as board)
- Libraries: **NimBLE-Arduino by h2zero** (via Arduino Library Manager) - do not use the default Bluedroid BLE library, NimBLE is significantly lighter
- Set `NANO_MAC` in the sketch to the helmet Nano's MAC address (printed at boot: `Helmet MAC: XX:XX:XX:XX:XX:XX`)
- Set `WIFI_CHANNEL` to match whatever channel the phone hotspot assigns at runtime (check helmet boot log: `[WiFi] Channel: X`)

The scanner uses no WiFi association - it calls `WiFi.mode(WIFI_STA)` + `WiFi.disconnect()` purely to initialise the radio for ESP-NOW, then explicitly sets the channel via `esp_wifi_set_channel()`. BLE scanning and ESP-NOW forwarding run independently on the same chip without conflict.

## What's Working

- Flask mock API with manager dashboard, 3 robots, command endpoint, forwards commands to robots
- Dobot Magician Go integrated via DobotLab Python file-polling bridge (forward, pick object)
- Wrist display:
  - PIN authentication with two roles: OPERATOR (can send commands) and VIEWER (browse only)
  - Role shown in header ("SMART HELMET HMI - OPERATOR / VIEWER")
  - Logout button (bottom-right of robot list screen)
  - Robot list (fetched from `/robots`), robot detail screen with status colour mapping
  - Dynamic command buttons based on robot capabilities; "View only" notice for VIEWER role
  - Command POST end-to-end with "Sent ✓" / error feedback label
  - Pick object command for Dobot
  - ESP-NOW scan flow: SCAN button → helmet → HuskyLens → robot ID → wrist switches to detail screen
  - "Scanning..." modal overlay with 6s timeout and error states
  - Vibration motor on GPIO 21 driven by helmet alarm-state ESP-NOW messages (3 danger tiers)
- Helmet:
  - WiFi + ESP-NOW transport (channel hardcoded in `config.h` - runtime channel detection via `WiFi.channel()` planned but not yet implemented, see Known Limitations)
  - HuskyLens Tag Recognition - real scan responses (largest tag in frame wins, 3s timeout, 100ms poll)
  - Safety bubble - BLE RSSI-based proximity detection via dedicated scanner board, sends alarm-state messages to wrist via ESP-NOW
  - `AlarmStateMsg` struct (4 bytes, msgType 30) disambiguated from scan `EspNowMessage` (2 bytes) and scanner beacon `ScannerBeacon` (4 bytes, msgType 20) by packet length then msgType
- BLE Scanner (LOLIN32):
  - Passively scans BLE advertisements, filters on manufacturer ID `0xFFFF` + version byte `0x01`
  - Reads RSSI per advertisement, forwards `robotId + dangerLevel + rssi` to Nano over ESP-NOW
  - Runs independently of helmet Nano - dedicated radio, no coexistence issues

## What's Planned

- Demo robot firmware (motor control via HTTP from Flask, BLE advertiser for safety bubble) - in development by a group member
- Robot list live status refresh on wrist display
- mDNS for stable Flask URL across network sessions (current workaround: static IP on laptop Wi-Fi adapter, but phone hotspot subnet can change between sessions)
- `lv_conf.h` / `User_Setup.h` reference copies in repo

## Known Limitations

- **Hotspot IP drift:** Android hotspots can change subnet between sessions. `API_BASE_URL` in `config.h` may need updating if Flask starts on a different IP. mDNS is the proper fix.
- **Hotspot channel drift:** Android hotspots do not always use a fixed channel. The helmet Nano reads the actual channel at runtime (`WiFi.channel()` after connect) and uses that for ESP-NOW. The BLE scanner has no WiFi association and uses a hardcoded channel - if the hotspot changes channel between sessions, the scanner's `WIFI_CHANNEL` constant must be updated and the scanner reflashed.
- **Safety bubble fail-open on loitering:** the helmet sends `alarmActive=1` once on alarm entry (no periodic heartbeat). The wrist auto-clears after a 10s cutoff. If a dangerous robot remains inside the bubble continuously for longer than 10s, the wrist haptic alert will stop at the 10s mark even though the helmet is still actively alarming. Accepted limitation for the demo; a production implementation would use a periodic heartbeat pattern.
- **Safety bubble fail-open on beacon loss:** if the robot's BLE advertisement stops (robot powered off, RF loss), the helmet clears the alarm after 3s beacon timeout. The robot may still be physically present. Acceptable for a demo; a production system would fail-closed (hold alarm on signal loss).
- **ESP-NOW 250-byte payload limit:** prevents helmet from sending full robot JSON over ESP-NOW. Wrist fetches robot details directly from Flask using the ID returned by the helmet scan.
- **HuskyLens proximity vs. BLE RSSI:** HuskyLens is used only for robot *identification* (scan flow). Proximity detection uses BLE RSSI, which is omnidirectional (works behind the worker) but coarse (±several metres depending on antenna, body absorption, environment). Thresholds (−70/−78 dBm) require empirical calibration per deployment venue.
- **Command ACK = receipt, not completion:** wrist shows "Sent ✓" when Flask returns HTTP 200, not when the robot actually executes the command. No completion confirmation loop exists.
- **Alternate-scan bug:** every other scan may show "No response from helmet" immediately on press - a state-leak bug between scan attempts, under investigation.
- **`lv_conf.h` / `User_Setup.h` not in repo:** required for wrist build but live in Arduino libraries folder. Fresh-machine build will fail without manual setup.
- **Dead code in `espnow_manager.h`:** `RobotBeacon` struct and `ESPNOW_MSG_ROBOT_BEACON` (msgType 10) are unused - left over from an earlier ESP-NOW-based beacon design that was superseded by the BLE scanner approach. Scheduled for cleanup.
