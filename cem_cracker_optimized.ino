/* SPDX-License-Identifier: GPL-3.0 */
/*
 * Optimized version for CEM-H P2 PIN cracking
 * For Teensy 4.0
 */

#include "cem_cracker_optimized.h"
#include <FlexCAN_T4.h>
#include <LiquidCrystal.h>

// Initialize LCD
const uint8_t rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println("CEM-H P2 PIN Cracker - Optimized Version");
  
  // Initialize LCD
  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.clear();
  lcd_printf(0, 0, "Initializing...");
  
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
  
  // Set CPU speed
  set_arm_clock(180000000);
  
  // Allow time for initialization
  delay(1000);
  
  // Read CEM part number
  uint32_t pn = ecu_read_part_number(CAN_LS, CEM_LS_ECU_ID);
  
  if (!pn) {
    pn = ecu_read_part_number(CAN_HS, CEM_HS_ECU_ID);
  }
  
  if (is_cem_h_p2(pn)) {
    lcd_printf(0, 0, "CEM-H P2: %lu", pn);
    optimize_cem_h_parameters();
  } else {
    lcd_printf(0, 0, "Not CEM-H P2!");
    lcd_printf(0, 1, "Check connection");
    while(1) delay(1000); // Stop here
  }
  
  // Enter programming mode
  progModeOn();
  
  // Ready to start
  lcd_printf(0, 1, "Ready to crack");
  delay(2000);
}

void loop() {
  static uint8_t pin[PIN_LEN] = {0};
  static bool cracking = false;
  
  // Start cracking when abort button is pressed and released
  if (!digitalRead(ABORT_PIN)) {
    delay(50); // Debounce
    while(!digitalRead(ABORT_PIN)); // Wait for release
    cracking = !cracking; // Toggle cracking state
    
    if (cracking) {
      lcd_printf(0, 1, "Cracking...");
      // Start optimized cracking process
      if (crack_cem_h_p2_pin(pin)) {
        lcd_printf(0, 0, "PIN Found!");
        cracking = false;
      }
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
