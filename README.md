# Wire-Bender

This repository contains the firmware and web interface for operating an automated wire bending machine.

# Operating the Machine

1. **Power On**: Turn on the power switch to start the machine.
2. **First-Time Wi-Fi Setup**: 
   - When powered on for the very first time, the machine will broadcast its own Wi-Fi network called **"WireBender-Setup"**.
   - Connect your phone or laptop to this **"WireBender-Setup"** network.
   - A setup page should pop up automatically. (If it doesn't, open a web browser and go to `192.168.4.1`).
   - Select your home Wi-Fi network, enter your password, and save. The machine will now connect to your home network automatically every time you turn it on. You only need to do this once.
3. **Wait for Homing**: After switching the machine on, **wait a few seconds**. The machine will immediately perform an automatic homing sequence. 
   - *What is Homing?* The motor will move the arm towards the limit switch until it is pressed. This establishes the arm's absolute "zero" position so it knows exactly where it is when moving.
4. **Use the Web Interface**: Once homing is complete, open the web interface on your computer or mobile device to control the machine manually or start wire bending experiment.

# Setup Instructions

For detailed information on building and configuring the hardware and software components, please refer to the following documents:

- **[Hardware Setup](hardware_setup.md)**
- **[Software Setup](software_setup.md)**

# Safety & Notes

- Always keep hands clear of moving parts during operation.
- Do not interfere with the machine or feed wire while it is performing its initial homing sequence.
