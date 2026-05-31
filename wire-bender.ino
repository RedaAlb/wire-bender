#include <WiFi.h>
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include <AccelStepper.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Load secrets (this file is ignored by Git)
#include "secrets.h"

// --- Hardware Pins ---
// Adjust these to match your actual ESP32 wiring
#define STEP_PIN 26
#define DIR_PIN 25
#define LIMIT_SWITCH_PIN 33
const bool USE_LIMIT_SWITCH = true;  // Set to true to use automatic homing
const int HOMING_BACKOFF_STEPS = 10; // Steps to reverse after hitting limit switch (tweak to just release the switch)

// --- Motor Settings ---
// Assuming a NEMA 17 with A4988 set to 4x microstepping
#define STEPS_PER_REV 800 
// High speed/acceleration needed for 2 cycles per second (0 -> 120 -> 0)
#define MAX_SPEED 2000
#define ACCELERATION 7500

FirebaseData streamData;
FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

String currentState = "stopped";
int currentCount = 0;
int targetCount = 100000;
float startAngle = 90.0;
float targetAngle = -120.0;
float manualAngle = 0.0;

// Flags for motor logic
bool movingDown = false;
bool isHomed = false;
bool preparingToStart = false;
bool isIdle = true;

void streamCallback(FirebaseStream data) {
  // Ignore all Firebase commands until the machine has finished homing
  if (!isHomed) return;

  String path = data.dataPath();
  
  if (path == "/state") {
    currentState = data.stringData();
    if(currentState == "stopped") {
      stepper.stop(); // Stop safely using deceleration
      currentCount = 0; // reset
      preparingToStart = false;
      isIdle = true;
    } else if (currentState == "paused") {
      stepper.stop(); // Stop safely using deceleration
    } else if (currentState == "running") {
      if (isIdle) {
        isIdle = false;
        // Ensure we start a new movement towards target if the motor was idle
        if (stepper.currentPosition() != angleToSteps(startAngle)) {
          preparingToStart = true;
          movingDown = false;
          moveToAngle(startAngle);
        } else {
          preparingToStart = false;
          movingDown = true;
          moveToAngle(targetAngle);
        }
      } else {
        // Resume from pause
        if (preparingToStart) {
          moveToAngle(startAngle);
        } else if (movingDown) {
          moveToAngle(targetAngle);
        } else {
          moveToAngle(startAngle);
        }
      }
    } else if (currentState == "manual") {
      isIdle = true;
      moveToAngle(manualAngle);
    }
  } else if (path == "/target_count") {
    targetCount = data.intData();
  } else if (path == "/target_angle") {
    targetAngle = data.floatData();
  } else if (path == "/start_angle") {
    startAngle = data.floatData();
  } else if (path == "/manual_angle") {
    manualAngle = data.floatData();
    if(currentState == "manual") {
      moveToAngle(manualAngle);
    }
  } else if (path == "/current_count") {
    if (data.intData() == 0) {
      currentCount = 0;
    }
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("Firebase stream timed out, resuming...");
}

void setup() {
  Serial.begin(115200);
  
  // IMMEDIATELY set stepper pins to OUTPUT and LOW to prevent them from floating.
  // Floating pins act as antennas and will pick up the ESP32's WiFi RF noise,
  // causing the stepper to randomly "jitter" or move during the WiFi connection phase.
  pinMode(STEP_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);
  
  // Limit switch setup. Assuming NO (Normally Open) switch pulling to GND when pressed.
  pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);
  
  // WiFiManager handles WiFi setup via Captive Portal
  WiFiManager wm;
  // wm.resetSettings(); // Un-comment this line once to wipe saved wifi credentials if needed
  wm.autoConnect("WireBender-Setup");
  Serial.println("Connected to WiFi!");

  // Firebase Setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Setup Firebase Stream to listen for changes from the Web UI asynchronously
  if (!Firebase.RTDB.beginStream(&streamData, "/wireBender")) {
    Serial.println("Firebase Stream failed");
  } else {
    Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
  }

  // Stepper Config
  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);

  // Fetch initial configuration from Firebase
  if(Firebase.RTDB.getInt(&fbData, "/wireBender/current_count")) {
    currentCount = fbData.intData();
  }
  if(Firebase.RTDB.getInt(&fbData, "/wireBender/target_count")) {
    targetCount = fbData.intData();
  }
  if(Firebase.RTDB.getFloat(&fbData, "/wireBender/start_angle")) {
    startAngle = fbData.floatData();
  }
  if(Firebase.RTDB.getFloat(&fbData, "/wireBender/target_angle")) {
    targetAngle = fbData.floatData();
  }
  if(Firebase.RTDB.getFloat(&fbData, "/wireBender/manual_angle")) {
    manualAngle = fbData.floatData();
  }

  String previousState = "stopped";
  if(Firebase.RTDB.getString(&fbData, "/wireBender/state")) {
    previousState = fbData.stringData();
  }

  // Signal to the UI that the machine is initializing
  currentState = "homing";
  isIdle = true;
  Firebase.RTDB.setString(&fbData, "/wireBender/state", "homing");

  homeStepper();

  // Machine is physically at 90 degrees now. Force the UI to sync to this reality.
  manualAngle = 90.0;
  Firebase.RTDB.setFloat(&fbData, "/wireBender/manual_angle", 90.0);

  // Resume previous state if appropriate
  if (previousState == "running") {
    currentState = "running";
    Firebase.RTDB.setString(&fbData, "/wireBender/state", "running");
    
    isIdle = false;
    if (stepper.currentPosition() != angleToSteps(startAngle)) {
      preparingToStart = true;
      movingDown = false;
      moveToAngle(startAngle);
    } else {
      preparingToStart = false;
      movingDown = true;
      moveToAngle(targetAngle);
    }
  } else if (previousState == "paused") {
    currentState = "paused";
    Firebase.RTDB.setString(&fbData, "/wireBender/state", "paused");
    isIdle = true;
  } else {
    // Machine is ready — signal the UI
    currentState = "stopped";
    Firebase.RTDB.setString(&fbData, "/wireBender/state", "stopped");
  }
}

void homeStepper() {
  if (USE_LIMIT_SWITCH) {
    Serial.println("Homing Stepper via limit switch...");
    stepper.setMaxSpeed(500);
    stepper.setSpeed(500);    // Gentle speed toward the limit switch (90 degrees)
    
    // Move until the switch triggers (goes LOW when pressed)
    while(digitalRead(LIMIT_SWITCH_PIN) == HIGH) {
      stepper.runSpeed();
      yield(); // Prevent watchdog reset during long homing phase
    }
    
    // Mark the switch contact point as 90 degrees
    stepper.setCurrentPosition(angleToSteps(90));
    Serial.println("Switch hit at 90 degrees. Backing off...");
    
    // Back off just enough steps to release the limit switch
    stepper.setMaxSpeed(MAX_SPEED);
    stepper.move(-HOMING_BACKOFF_STEPS);
    while(stepper.distanceToGo() != 0) {
      stepper.run();
      yield();
    }
    // Treat this position as 90 degrees (the offset is negligible)
    stepper.setCurrentPosition(angleToSteps(90));
    
    isHomed = true;
    Serial.println("Homed successfully at 90 degrees.");
  } else {
    Serial.println("Homing bypassed for testing. Assuming current position is 90 degrees.");
    stepper.setCurrentPosition(angleToSteps(90));
    isHomed = true;
    Serial.println("Manual homing successful.");
  }
}

long angleToSteps(float angle) {
  return (long)((angle / 360.0) * STEPS_PER_REV);
}

void moveToAngle(float angle) {
  stepper.moveTo(angleToSteps(angle));
}

void loop() {
  Firebase.ready(); // handles token generation
  
  if (currentState == "running") {
    // If the motor has reached its target position
    if (stepper.distanceToGo() == 0) {
      if (preparingToStart) {
        // We just returned to startAngle to prepare for the test, now begin actual cycle
        preparingToStart = false;
        movingDown = true;
        moveToAngle(targetAngle);
      } else if (movingDown) {
        // We reached targetAngle, now return to startAngle
        moveToAngle(startAngle);
        movingDown = false;
      } else {
        // We reached startAngle, one full stretch cycle is complete!
        currentCount++;
        
        if (currentCount >= targetCount) {
          // Final count update (Synchronous to ensure delivery)
          Firebase.RTDB.setInt(&fbData, "/wireBender/current_count", currentCount);
          
          // Experiment finished — record end time via server timestamp
          // so duration is accurate even if the Web UI was closed.
          currentState = "stopped";
          isIdle = true;
          Firebase.RTDB.setTimestamp(&fbData, "/wireBender/end_time");
          Firebase.RTDB.setString(&fbData, "/wireBender/state", "stopped");
        } else {
          // Intermediate count update (Async so we don't block the motor)
          if (currentCount % 2 == 0) {
            Firebase.RTDB.setIntAsync(&fbData, "/wireBender/current_count", currentCount);
          }
          
          // Start next cycle
          moveToAngle(targetAngle);
          movingDown = true;
        }
      }
    }
  }
  
  // Always step the motor if there is a target (handles running, manual, and graceful deceleration when paused/stopped)
  stepper.run();
}
