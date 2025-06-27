/* SPDX-License-Identifier: GPL-3.0 */
/*
 * Main sketch file for CEM-H P2 PIN cracker
 * Optimized version for Teensy 4.0
 */

#include "cem_cracker_optimized.h"

// Initialize LCD
const uint8_t rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Custom characters for spinner animation
const uint8_t spinnerChars[4][8] = {
    {0x00,0x0F,0x0F,0x0F,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x1E,0x1E,0x1E,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x1C,0x1C,0x1C,0x1C},
    {0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}
};

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println(F("CEM-H P2 PIN Cracker - Optimized Version"));
    
    // Initialize LCD
    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.clear();
    
    // Create custom characters for spinner
    for (int i = 0; i < 4; i++) {
        lcd.createChar(i, (uint8_t*)spinnerChars[i]);
    }
    
    lcd_printf(0, 0, "Initializing...");
    
    // Enable ARM cycle counter for precise timing
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
    
    // Initialize CAN buses
    can_hs.begin();
    can_ls.begin();
    
    // Set up CAN bus parameters
    can_hs.setBaudRate(CEM_H_P2_BAUD);
    can_ls.setBaudRate(CAN_125KBPS);
    
    // Enable interrupts and FIFO
    can_hs.enableFIFO();
    can_hs.enableFIFOInterrupt();
    can_hs.onReceive(can_hs_event);
    
    can_ls.enableFIFO();
    can_ls.enableFIFOInterrupt();
    can_ls.onReceive(can_ls_event);
    
    // Set up pins
    pinMode(CAN_L_PIN, INPUT_PULLUP);
    pinMode(CALC_BYTES_PIN, INPUT_PULLUP);
    pinMode(ABORT_PIN, INPUT_PULLUP);
    
    // Attach abort interrupt
    attachInterrupt(digitalPinToInterrupt(ABORT_PIN), abortIsr, LOW);
    
    // Set CPU speed to maximum
    set_arm_clock(600000000);
    
    // Allow time for initialization
    delay(1000);
    
    // Read CEM part number
    uint32_t pn = 0;
    lcd_printf(0, 1, "Reading CEM...");
    
    // Try reading part number from low-speed bus first
    pn = ecu_read_part_number(CAN_LS, CEM_LS_ECU_ID);
    
    // If not found, try high-speed bus
    if (!pn) {
        pn = ecu_read_part_number(CAN_HS, CEM_HS_ECU_ID);
    }
    
    // Verify we have a CEM-H P2
    if (is_cem_h_p2(pn)) {
        lcd_printf(0, 0, "CEM-H P2: %lu", pn);
        optimize_cem_h_parameters();
    } else {
        lcd_printf(0, 0, "Not CEM-H P2!");
        lcd_printf(0, 1, "Check connection");
        while(1) delay(1000); // Stop here
    }
    
    // Enter programming mode
    lcd_printf(0, 1, "Enter PROG mode");
    progModeOn();
    
    // Ready to start
    lcd_printf(0, 1, "Press to start");
}

void loop() {
    static uint8_t pin[PIN_LEN] = {0};
    static bool cracking = false;
    
    // Start/stop cracking when abort button is pressed and released
    if (!digitalRead(ABORT_PIN)) {
        delay(50); // Debounce
        while(!digitalRead(ABORT_PIN)); // Wait for release
        cracking = !cracking; // Toggle cracking state
        
        if (cracking) {
            lcd_printf(0, 1, "Cracking...");
            memset(pin, 0, sizeof(pin)); // Clear PIN
            
            // Start optimized cracking process
            if (crack_cem_h_p2_pin(pin)) {
                lcd_printf(0, 0, "PIN Found!");
                lcd_printf(0, 1, "%02X%02X%02X%02X%02X%02X",
                          pin[0], pin[1], pin[2],
                          pin[3], pin[4], pin[5]);
                          
                // Verify PIN
                uint32_t latency;
                if (cemUnlock(pin, NULL, &latency, true)) {
                    Serial.println(F("PIN verified successfully!"));
                } else {
                    Serial.println(F("PIN verification failed!"));
                }
            } else {
                lcd_printf(0, 0, "Failed/Aborted");
            }
            cracking = false;
        } else {
            lcd_printf(0, 1, "Aborted!");
            abortReq = true;
        }
    }
    
    // Process CAN events
    can_hs.events();
    can_ls.events();
    
    // Update display spinner when cracking
    if (cracking) {
        lcd_spinner();
    }
    
    delay(10); // Small delay to prevent overwhelming the system
}
