/* SPDX-License-Identifier: GPL-3.0 */
/*
 * Header file for optimized CEM-H P2 PIN cracking
 */

#ifndef CEM_CRACKER_OPTIMIZED_H
#define CEM_CRACKER_OPTIMIZED_H

#include <Arduino.h>
#include <stdint.h>
#include <FlexCAN_T4.h>
#include <LiquidCrystal.h>

// CAN bus configuration
#define CAN_500KBPS 500000
#define CAN_250KBPS 250000
#define CAN_125KBPS 125000

// CEM ECU IDs
#define CEM_HS_ECU_ID   0x50
#define CEM_LS_ECU_ID   0x40

// Message sizes
#define CAN_MSG_SIZE    8
#define PIN_LEN         6

// LCD configuration
#define LCD_ROWS        2
#define LCD_COLS        16

// Pin definitions
#define CAN_L_PIN      PIND2
#define CALC_BYTES_PIN PIND3
#define ABORT_PIN      14

// CEM-H P2 Specific configurations
#define CEM_H_P2_BAUD CAN_500KBPS
#define CEM_H_P2_SHUFFLE_ORDER 1

// Timing parameters
#define CEM_H_REPLY_MIN_FACTOR 0.4
#define CEM_H_REPLY_MAX_FACTOR 1.3

// Batch processing configuration
#define BATCH_SIZE 10
#define MIN_SAMPLES_PER_BATCH 50
#define MAX_SAMPLES_FIRST_POSITION 300

// Early termination thresholds
#define CONFIDENCE_THRESHOLD 0.85
#define MIN_VALID_SAMPLES 5
#define QUICK_REJECT_THRESHOLD 10
#define QUICK_TIMEOUT_FACTOR 0.4

// Statistical analysis parameters
struct pin_statistics {
    uint32_t total_latency;
    uint32_t valid_samples;
    double mean_latency;
    double std_deviation;
    double confidence_score;
};

// Batch processing structure
struct pin_batch {
    uint8_t start_value;
    uint8_t end_value;
    pin_statistics stats;
    bool processed;
};

// CAN bus types
typedef enum {
    CAN_HS,
    CAN_LS
} can_bus_id_t;

// External variable declarations
extern FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can_hs;
extern FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can_ls;
extern uint32_t cem_reply_min;
extern uint32_t cem_reply_avg;
extern uint32_t cem_reply_max;
extern volatile bool abortReq;
extern uint8_t shuffle_order[PIN_LEN];

// Function declarations
bool is_cem_h_p2(uint32_t part_number);
void optimize_cem_h_parameters(void);
double calculate_confidence_score(const pin_statistics *stats);
void process_pin_batch(pin_batch *batch, uint8_t *pin, uint32_t pos);
bool should_terminate_early(const pin_statistics *stats);
bool crack_cem_h_p2_pin(uint8_t *pin);
void lcd_printf(uint8_t x, uint8_t y, const char *fmt, ...);
void lcd_spinner(void);
bool cemUnlock(uint8_t *pin, uint8_t *pinUsed, uint32_t *latency, bool verbose);
void canMsgSend(can_bus_id_t bus, uint32_t id, uint8_t *data, bool verbose);
bool canMsgReceive(can_bus_id_t bus, uint32_t *id, uint8_t *data, uint32_t wait, bool verbose);
uint8_t binToBcd(uint8_t value);
uint8_t bcdToBin(uint8_t value);
void can_hs_event(const CAN_message_t &msg);
void can_ls_event(const CAN_message_t &msg);
void abortIsr(void);

#endif /* CEM_CRACKER_OPTIMIZED_H */
