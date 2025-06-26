/* SPDX-License-Identifier: GPL-3.0 */
/*
 * Header file for optimized CEM-H P2 PIN cracking
 */

#ifndef CEM_CRACKER_OPTIMIZED_H
#define CEM_CRACKER_OPTIMIZED_H

// CEM-H P2 Specific configurations
#define CEM_H_P2_BAUD CAN_500KBPS
#define CEM_H_P2_SHUFFLE_ORDER 1  // Uses shuffle order (3,1,5,0,2,4)

// Optimized timing parameters
#define CEM_H_REPLY_MIN_FACTOR 0.4  // More aggressive minimum threshold
#define CEM_H_REPLY_MAX_FACTOR 1.3  // Extended maximum threshold

// Batch processing configuration
#define BATCH_SIZE 10
#define MIN_SAMPLES_PER_BATCH 50
#define MAX_SAMPLES_FIRST_POSITION 300

// Early termination thresholds
#define CONFIDENCE_THRESHOLD 0.85
#define MIN_VALID_SAMPLES 5
#define QUICK_REJECT_THRESHOLD 10

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

// Function prototypes
bool is_cem_h_p2(uint32_t part_number);
void optimize_cem_h_parameters(void);
double calculate_confidence_score(const pin_statistics *stats);
void process_pin_batch(pin_batch *batch, uint8_t *pin, uint32_t pos);
bool should_terminate_early(const pin_statistics *stats);

#endif /* CEM_CRACKER_OPTIMIZED_H */
