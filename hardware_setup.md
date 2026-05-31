# Wire Bender - Hardware Setup Guide

## 1. Required Components
- **Microcontroller:** AZDelivery ESP32-DevKitC NodeMCU WiFi WLAN CP2102 ESP32-WROOM-32D
- **Stepper Motor:** Nema 17 Stepper Motor (17HS4401)
- **Motor Driver:** A4988 (PoPprint 5Pcs Ramps1.4 A4988 Stepper Motor Driver)
- **Limit Switch:** Standard mechanical limit switch (Normally Open configuration)
- **Power Supply:** 12V 5A DC Wall Power Supply
- **Voltage Regulator:** LM2596 DC-DC Buck Converter (to step down 12V to 5V for the ESP32)

> [!WARNING]
> **Why A4988 over L298N?**
> The A4988 is the only correct choice for this project. The L298N is an outdated H-Bridge driver meant primarily for DC motors. It lacks microstepping and current limiting control. Using an L298N with a bipolar stepper motor like the 17HS4401 will cause the motor to overheat, lose steps, run loudly, and perform poorly, especially at the high speeds (8000 max speed) required for the 2Hz cycle in this project. The A4988 is a dedicated stepper driver that solves all these issues.

## 2. Wiring & Pin Configuration

### ESP32 to A4988 Stepper Driver
Based on the defined pins in `wire-bender.ino`:
- **ESP32 Pin 26 (GPIO 26)** ➜ **A4988 STEP Pin**
- **ESP32 Pin 25 (GPIO 25)** ➜ **A4988 DIR Pin**
- **ESP32 GND** ➜ **A4988 GND (Logic GND)**
- **ESP32 3V3** ➜ **A4988 VDD (Logic Power)**

> [!TIP]
> **Microstepping Configuration:** The software is configured for **4x microstepping** (800 steps/rev). This is the optimal "sweet spot" for this project. It drastically reduces the processing load on the ESP32 while maintaining smooth enough motion to prevent violent vibrations. To enable 4x microstepping on the A4988, connect **MS2 to Logic High (3.3V)**, and leave **MS1 and MS3 disconnected (or tied to GND)**. (Note: Connecting MS1 to High instead of MS2 puts the driver in 2x/Half Step mode, which will cause the motor to move twice as far as expected). Also, tie the **RESET** and **SLEEP** pins together on the A4988.

### Preventing Overheating (Current Limiting)
> [!WARNING]
> Microstepping does **not** control heating. To prevent the Nema 17 and A4988 from overheating, you **must** tune the current limit potentiometer on the A4988.
> 1. Power the A4988 (logic and motor power).
> 2. Carefully measure the DC voltage between the small metal screw (potentiometer) on the A4988 and GND.
> 3. Use a small screwdriver to adjust the screw until the voltage reads around **0.6V to 0.8V** (providing roughly 1A to 1.2A of current, which is safe for most Nema 17s).

### A4988 to Nema 17 Stepper Motor (17HS4401)
You have a 4-wire stepper motor with **Red, Green, Yellow, and Blue** wires. A stepper motor has two internal coils, and we need to match the pairs to the A4988 pins (1A/1B for Coil 1, and 2A/2B for Coil 2). 

> [!TIP]
> **The Wire-Pairing Trick:** To be 100% sure which wires are pairs, take two unpowered wires and touch their metal ends together. While holding them together, try to spin the motor shaft with your fingers. If it becomes noticeably harder to spin, those two wires belong to the same coil! 

Usually, for a Red/Green/Yellow/Blue cable, the pairs are adjacent. Assuming Red/Green is Coil 1 and Yellow/Blue is Coil 2, wire them like this:
- **A4988 1B** ➜ **Red**
- **A4988 1A** ➜ **Green**
- **A4988 2A** ➜ **Yellow**
- **A4988 2B** ➜ **Blue**

*(Note: If the motor spins in the opposite direction than expected when you run the code, simply power down the driver and swap the position of the Red and Green wires).*

### Power Supply & LM2596 Wiring
This project uses a single 12V 5A wall adapter to power both the motor (directly) and the ESP32 (via the LM2596 buck converter).

**1. Wiring the Motor Driver (12V):**
- **12V Power Supply (+)** ➜ **A4988 VMOT**
- **12V Power Supply (-)** ➜ **A4988 GND (Motor GND)**

> [!CAUTION]
> Add a capacitor (e.g., 100µF or 47µF) across VMOT and Motor GND as close to the A4988 board as possible. This protects the driver from voltage spikes. **Never disconnect or connect the stepper motor while the driver is powered.** Doing so will instantly destroy the A4988 chip.

**2. Wiring the ESP32 (5V via LM2596):**
> [!WARNING]
> **TUNE THE LM2596 BEFORE CONNECTING THE ESP32!**
> Connect the 12V power to the LM2596 IN+ and IN-. Use a multimeter to measure the OUT+ and OUT- pins. Turn the small brass screw on the blue potentiometer counter-clockwise until the output voltage reads exactly **5.0V**.

- **12V Power Supply (+)** ➜ **LM2596 IN+**
- **12V Power Supply (-)** ➜ **LM2596 IN-**
- **LM2596 OUT+ (5V)** ➜ **ESP32 5V (or VIN) Pin**
- **LM2596 OUT- (GND)** ➜ **ESP32 GND**

### ESP32 to Limit Switch
The limit switch is used for homing the machine at startup. It uses the ESP32's internal pull-up resistor.
- **ESP32 Pin 33 (GPIO 33)** ➜ **Limit Switch (NO - Normally Open pin)**
- **ESP32 GND** ➜ **Limit Switch (COM - Common pin)**
