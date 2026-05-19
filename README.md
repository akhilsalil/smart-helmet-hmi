# Smart Helmet (readme to be updated in future commits)
> Collaborators - Akhil Salil, Alby George, Alan Francis, Jerrin Johnson, Noel Gigo

A wearable human-machine interface prototype that allows construction workers to identify robots on site, view their current status, and send operational commands via a helmet-mounted camera and a wrist-mounted display.

## System Components

| Component                            | Status            |
|--------------------------------------|---------------------|
| Mock Service (Flask API + Dashboard) | Nearly done         |
| Wrist Display (ESP32 touch UI)       | In dev              |
| Helmet Unit (Arduino + HuskyLens)    | In discussion       |

## How It Works
1. Worker wears helmet with HuskyLens camera mounted
2. Worker triggers a scan via wrist display
3. HuskyLens identifies the robot model
4. Helmet Arduino fetches robot info and live status from the mock service
5. Wrist display shows robot details and available commands
6. Worker sends a command — robot responds, mock service updates, dashboard reflects the change in real time

## Repository Structure
```
smart-helmet-hmi/
├── mockService/       # Flask REST API + web dashboard
├── wristDisplay/      # ESP32 touch display UI (Arduino)
├── helmet/            # Romeo Mini + HuskyLens code (Arduino)
└── README.md
```

## Running the Mock Service
See [mockService/README.md](mockService/README.md) for full setup instructions.

## Hardware
- HuskyLens V1.1 (robot identification)
- DFRobot Romeo Mini ESP32-C3 (helmet brain)
- DIYmalls ESP32-3248S035C 3.5" touch display (wrist unit) (add amazon link before commit)
- 2 demo robot - havent decided which ones yet
