# Volvo CEM PIN Cracker - Optimized for CEM-H P2
## Installation Instructions

### Prerequisites
1. Arduino IDE (latest version)
2. Teensyduino add-on for Arduino IDE
3. Required libraries:
   - FlexCAN_T4
   - LiquidCrystal

### Setup Steps

1. Install Arduino IDE
   - Download from https://www.arduino.cc/en/software
   - Install for your operating system

2. Install Teensyduino
   - Download from https://www.pjrc.com/teensy/td_download.html
   - Install the add-on following the instructions for your OS

3. Install Required Libraries
   - Open Arduino IDE
   - Go to Tools -> Manage Libraries
   - Search for and install:
     * FlexCAN_T4
     * LiquidCrystal

### Loading the Code

1. Copy these files to your Arduino sketch folder:
   - cem_cracker_optimized.cpp (rename to cem_cracker_optimized.ino)
   - cem_cracker_optimized.h
   - cem_h_p2_optimizations.cpp

2. Configure Arduino IDE:
   - Select Tools -> Board -> Teensyduino -> Teensy 4.0
   - Set CPU Speed to 180 MHz
   - Set Optimize to "Faster"

3. Upload:
   - Connect your Teensy 4.0 via USB
   - Press the upload button (→) in Arduino IDE
   - Press the reset button on Teensy if prompted

### Hardware Connections

Ensure proper connections:
- LCD Display:
  * RS pin to digital pin 8
  * Enable pin to digital pin 9
  * D4 pin to digital pin 4
  * D5 pin to digital pin 5
  * D6 pin to digital pin 6
  * D7 pin to digital pin 7

- CAN Bus:
  * CAN High to Teensy CAN1 TX
  * CAN Low to Teensy CAN1 RX
  * Use proper 120Ω termination resistors

### Testing

1. After upload, the LCD should show:
   - "Initializing..." message
   - Then display CEM part number when connected

2. Verify CAN communication:
   - LED should blink during CAN activity
   - LCD should update with cracking progress

### Troubleshooting

If upload fails:
1. Press and hold Teensy reset button
2. Click upload in Arduino IDE
3. Release reset button when prompted

If CAN communication fails:
1. Check physical connections
2. Verify CAN bus termination
3. Check baud rate settings match your CEM

### Safety Notice

This tool is for educational and research purposes only. Use responsibly and legally.
