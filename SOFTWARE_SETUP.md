# Wire Bender - Arduino IDE Setup Guide

To flash the `wire-bender.ino` code to your ESP32, follow these steps in the Arduino IDE.

## 1. Install ESP32 Board Support
By default, the Arduino IDE doesn't know how to talk to an ESP32. You need to add the ESP32 core.
1. Open Arduino IDE and go to **File > Preferences**.
2. In the **Additional Boards Manager URLs** field, paste this URL:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Click **OK**.
4. Go to **Tools > Board > Boards Manager...**
5. Search for `esp32` and install the package named **esp32 by Espressif Systems**.

## 2. Select Your Board
Once the ESP32 core is installed, select your specific board:
1. Go to **Tools > Board > ESP32 Arduino**.
2. Select **DOIT ESP32 DEVKIT V1** or **ESP32 Dev Module** (Both work perfectly for the AZDelivery ESP32-WROOM-32D).
3. Ensure the settings under the **Tools** menu look roughly like this:
   - **Upload Speed:** 115200 (or 921600 if it supports it for faster uploads)
   - **Flash Frequency:** 80MHz

## 3. Install Required Libraries
The project uses several external libraries. You can install them by going to **Sketch > Include Library > Manage Libraries...** (or clicking the Library Manager icon on the left). 

Search for and install the following exact libraries:
- **`WiFiManager`** by *tzapu* (Used to create the "WireBender-Setup" WiFi portal so you don't hardcode WiFi passwords)
- **`Firebase Arduino Client Library for ESP8266 and ESP32`** by *Mobizt* (Used for database syncing)
- **`AccelStepper`** by *Mike McCauley* (Used for precise stepper motor control)

## 4. Setup `secrets.h`
Because your Firebase credentials shouldn't be publicly uploaded to GitHub, they are kept in a separate file.
1. In the project folder, you will see a file named `secrets.example.h`.
2. Duplicate it or rename it to `secrets.h`.
3. Open `secrets.h` and fill in your Firebase project's `API_KEY`, `DATABASE_URL`, `USER_EMAIL`, and `USER_PASSWORD`.

## 5. Upload the Code
1. Plug your ESP32 into your computer via a data-capable Micro-USB cable.
2. Go to **Tools > Port** and select the COM port that corresponds to your ESP32.
3. Click the **Upload** arrow at the top left.
4. **Important:** When you see `Connecting...` with dots and dashes appear in the bottom terminal, you may need to **press and hold the "BOOT" button** on your AZDelivery ESP32 board until it begins uploading.

Once uploaded, open the **Serial Monitor** (set baud rate to `115200`) to verify it is working. The ESP32 will broadcast a WiFi network called "WireBender-Setup". Connect your phone/laptop to that network, and a captive portal will pop up allowing you to enter your home WiFi credentials!
