/*
 * Copyright (c) 2025 Subbotin N.Y. <neymarus@yandex.ru>
 * GitHub: https://github.com/nktsb
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HR_ANALYZER_H_
#define __HR_ANALYZER_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define HR_ANALYZER_EMPTY    0.0f   ///< Returned when not enough data is available yet
#define HR_ANALYZER_ERRROR   -1.0f  ///< Returned on internal error

/**
 * @brief Enumeration for internal state of extrema detection.
 */
typedef enum {
    LOCAL_EXTM_IDLE,      ///< No extrema being tracked
    LOCAL_EXTM_STARTED,   ///< Tracking in progress
    LOCAL_EXTM_FOUND      ///< Local extremum found
} extrema_state_t;

/**
 * @brief Analyzer instance structure for heart rate detection.
 */
typedef struct pulsomteter {
    int32_t prev_sample_val;          ///< Previous sample value (used to detect slope changes)

    int32_t local_max_val;            ///< Detected local maximum value
    extrema_state_t local_max_state;  ///< State of local max tracking

    int32_t local_min_val;            ///< Detected local minimum value
    extrema_state_t local_min_state;  ///< State of local min tracking

    int32_t hysteresis;               ///< Current hysteresis value
    const int32_t hysteresis_div;     ///< Division factor for hysteresis calculation

    int32_t beat_threshold;           ///< Dynamic beat threshold (midpoint between extrema)
    uint32_t prev_beat_ts;            ///< Timestamp of previous detected beat

    const float heart_rate_max_val;   ///< Max allowed heart rate (bpm)
    const float heart_rate_min_val;   ///< Min allowed heart rate (bpm)

    float heart_rate_val;             ///< Last calculated heart rate (bpm)
} hr_analyzer_st;

/**
 * @brief Initializes a new heart rate analyzer instance.
 *
 * @param hysteresis_div Hysteresis divisor used in amplitude-based thresholding.
 * @return Pointer to new hr_analyzer_st instance. NULL on failure.
 */
hr_analyzer_st *hr_analyzer_init(int32_t hysteresis_div);

/**
 * @brief Frees analyzer resources.
 *
 * @param hr_analyzer Pointer to analyzer instance.
 */
void hr_analyzer_deinit(hr_analyzer_st *hr_analyzer);

/**
 * @brief Feeds a new sample into the heart rate analyzer and optionally returns a new BPM value.
 *
 * This function processes a single filtered PPG sample, detects local extrema,
 * and checks for a beat (threshold crossing on falling edge). If a new beat is detected,
 * it calculates and returns the new heart rate value via the output pointer.
 *
 * @param hr_analyzer Pointer to the heart rate analyzer instance.
 * @param hr_val Pointer to a float where the calculated BPM will be stored.
 *               If no new value is available, the last valid value is returned.
 * 				 If waiting new beat is timed out, *hr_val = HR_ANALYZER_EMPTY
 * @param new_sample_val New filtered PPG signal sample.
 * @param current_time_ms Current timestamp in milliseconds.
 *
 * @return `true` if a new BPM value was calculated (beat detected or timeout reset),
 *         `false` if no update occurred.
 */
bool hr_analyzer_process_sample(hr_analyzer_st *hr_analyzer, float *hr_val,
    int32_t new_sample_val, uint32_t current_time_ms);

#endif /* __HR_ANALYZER_H_ */
