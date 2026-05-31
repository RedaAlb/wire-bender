# Wire Bender Durability Tester - AI Agent Context

**Hello fellow AI Agent!** 
If you are reading this, you have been assigned to help with the "Wire Bender" project. This document contains everything you need to know to perfectly understand the project architecture, design decisions, and codebase so you can jump right in.

---

## 1. Project Overview & Goal
The project is a physical durability testing machine. It tests the stretch-ability and durability of a thin wire by repeatedly stretching it. 
- **Mechanism**: There are two rectangular arms (100x20x5mm). One is fixed (flat), the other is attached to a stepper motor.
- **Motion**: The moving arm rotates between a customizable start angle (default 90°) and target angle (default -120°). Home position is at 90° (moving arm straight up, forming a backwards L), 0° is flat, and -120° is down and to the left.
- **Speed**: It does a full cycle (e.g. 90 -> -120 -> 90) twice a second.
- **Repetitions**: A standard experiment runs for 100,000 cycles.

## 2. Hardware Architecture
- **Microcontroller**: ESP32
- **Motor**: NEMA 17 Stepper Motor
- **Motor Driver**: A4988 (uses standard STEP/DIR pins). Configured for 4x microstepping (800 steps/rev).
- **Homing**: A Limit Switch is used to automatically find the 90-degree (up) position when the ESP32 boots up.

## 3. Software Architecture

The software is split into two halves that communicate entirely via **Firebase Realtime Database**.

### A. ESP32 Firmware (`wire-bender.ino`)
- **WiFi**: Uses `tzapu/WiFiManager` to create a captive portal (`WireBender-Setup`) so WiFi credentials don't need to be hardcoded.
- **Motor Control**: Uses `AccelStepper`. The motor needs high acceleration (`7500`) and speed (`2000`) to hit the 2Hz cycle requirement.
- **Firebase Sync**: Uses the `Firebase ESP Client` (by Mobizt). 
  - To prevent network calls from blocking the high-speed motor stepping, it uses **Firebase Streams** (`Firebase.RTDB.beginStream`) to listen to state changes asynchronously.
  - It pushes the current repetition count to Firebase using `setIntAsync` every 2 cycles to conserve bandwidth.

### B. Web UI (`web-ui/index.html`)
- **Stack**: Pure Vanilla HTML, CSS, and JS. No React, no build steps. 
- **Styling**: Modern dark mode, glassmorphism UI with inter font.
- **Authentication**: Uses Firebase Anonymous Authentication (`signInAnonymously`) in the background so the user doesn't have to type a password, but the database remains secure.
- **Features**: 
  - Live progress bar, elapsed time, current/target counts.
  - Start/Pause/Stop controls.
  - Manual control input to command the arm to a specific angle.
  - History table that saves past experiments.

## 4. Firebase Database Schema & Security
The database uses Firebase Realtime Database locked with `auth != null` security rules.
**Data Structure:**
```json
{
  "wireBender": {
    "state": "stopped",       // Can be: "homing", "running", "paused", "stopped", "manual"
    "current_count": 0,       // Updated by ESP32 every cycle
    "target_count": 100000,   // Set by Web UI
    "start_angle": 90.0,      // Set by Web UI, start point of the rep
    "target_angle": -120.0,   // Set by Web UI, end point of the rep
    "manual_angle": 0.0,      // Set by Web UI, read by ESP32
    "experiment_name": "Test 1",
    "start_time": 1716500000000, // Epoch timestamp
    "end_time": null             // Set by Web UI when experiment completes or is stopped
  },
  "history": {
    "-Nabcd12345": {
      "experiment_name": "Test 1",
      "start_time": 1716500000000,
      "reps_achieved": 5000,
      "target": 100000,
      "start_angle": 90.0,
      "target_angle": -120.0,
      "duration": "01:23:45"
    }
  }
}
```

## 5. Secrets Management (Crucial)
To prevent leaking credentials to GitHub, we use `.gitignore` on two files. **DO NOT OVERWRITE THESE FILES OR REMOVE THEM FROM GITIGNORE.**
1. `secrets.h` (Arduino credentials). Template available at `secrets.example.h`.
2. `web-ui/firebase-config.js` (Web UI credentials). Template available at `web-ui/firebase-config.example.js`.

## 6. Current Status & Future Considerations
- **Status**: The code is fully written, configured, and ready for hardware testing.
- **Future Considerations**: 
  - Writing to Firebase 2 times a second might consume a lot of the Firebase Free Tier bandwidth over a 14-hour experiment. If the user hits limits, the ESP32 code should be modified to update Firebase every 50 or 100 cycles instead.
  - The limit switch homing logic assumes the switch goes LOW when pressed (`INPUT_PULLUP`). This might need inverting depending on physical wiring.
  - If the Web UI is closed when the ESP32 finishes an experiment, the ESP32 records `end_time` via a Firebase Server Timestamp, ensuring the duration saved to history is accurate regardless.
