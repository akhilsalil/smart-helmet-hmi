# Robot Dashboard Mock API
*Author: Akhil Salil

A lightweight, Flask-based mock API service for a robot management dashboard. This service uses local JSON files as a mock DB to serve robot configurations, track their current tasks, and accept operational commands.

## Context
This service is part of the Smart Helmet HMI project - aprototype developed 
at OTH Regensburg. It simulates the central robot management platform that would (likely) 
exist on a real construction site, allowing the smart helmet to fetch 
robot status and push commands without requiring proprietary robot APIs.

## Features
- Serves a frontend dashboard UI.
- Merges static robot configuration data with dynamic task status data.
- Handles `GET` requests for individual robot details and statuses.
- Handles `POST` requests to simulate sending commands to robots.

## Project Structure
Ensure your project directory looks like this before running the application:

```
├── app.py                 # Main Flask application
├── static/
│   └── dashboard.js       # Frontend JavaScript
├── templates/
│   └── dashboard.html     # Frontend Dashboard UI
└── data/
    ├── robots.json        # Static robot configurations
    └── tasks.json         # Dynamic robot statuses and tasks
```

## Prerequisites
- Python 3.6+
- Flask

## Installation & Setup

1. **Clone or download the repository.**
2. **Create a virtual environment (optional but recommended):**
   ```bash
   python -m venv .venv
   source .venv/bin/activate  # On Windows use: .venv\Scripts\activate
   ```
3. **Install Flask:**
   ```bash
   pip install flask
   ```
4. **Run the application:**
   ```bash
   python app.py
   ```
   The service will prolly(check later before commiting) start on `http://0.0.0.0:5000/`.

---

## API Documentation

### 1. Web UI Dashboard
* **URL:** `/`
* **Method:** `GET`
* **Description:** Renders the main `dashboard.html` template.

### 2. Get All Robots (Merged Data)
* **URL:** `/robots`
* **Method:** `GET`
* **Description:** Returns a list of all robots, merging their static configuration (from `robots.json`) with their current task/status (from `tasks.json`).
* **Success Response:** `200 OK` (Array of JSON objects)

### 3. Get Specific Robot Configuration
* **URL:** `/robot/<robot_id>`
* **Method:** `GET`
* **Description:** Retrieves the static configuration and capabilities of a specific robot by its ID.
* **Success Response:** `200 OK`
* **Error Response:** `404 Not Found` `{"error": "Robot not found"}`

### 4. Get Specific Robot Status
* **URL:** `/robot/<robot_id>/status`
* **Method:** `GET`
* **Description:** Retrieves the current task, battery life, and operational status of a specific robot.
* **Success Response:** `200 OK`
* **Error Response:** `404 Not Found` `{"error": "Status not found"}`

### 5. Send Command to Robot
* **URL:** `/robot/<robot_id>/command`
* **Method:** `POST`
* **Description:** Updates a robot's task status by issuing a new command. This modifies the `tasks.json` file.
* **Request Body:**
  ```json
  {
    "command": "Drive forward",
    "operator": "manager-console" // Optional, defaults to 'helmet-operator'
  }
  ```
* **Success Response:** `200 OK`
  ```json
  {
    "success": true,
    "robot_id": 2,
    "command": "Drive forward"
  }
  ```