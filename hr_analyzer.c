/*
 * Copyright (c) 2025 Subbotin N.Y. <neymarus@yandex.ru>
 * GitHub: https://github.com/nktsb
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hr_analyzer.h"

#define MS_PER_MINUTE			(1000 * 60)
#define HR_MAX_RES_VAL			240.0f
#define HR_MIN_RES_VAL			40.0f
#define HR_ANALYZER_DEF_HYST_DIV	5
#define RESET_ANALYZER_TMO_MS		2000


static void hr_analyzer_reset_local_max_min(hr_analyzer_st *hr_analyzer)
{
	hr_analyzer->local_max_val = 0;
	hr_analyzer->local_max_state = LOCAL_EXTM_IDLE;

	hr_analyzer->local_min_val = 0;
	hr_analyzer->local_min_state = LOCAL_EXTM_IDLE;

	hr_analyzer->beat_threshold = 0;
}

static void hr_analyzer_set_new_hysteresis(hr_analyzer_st *hr_analyzer)
{
	hr_analyzer->hysteresis = (hr_analyzer->local_max_val - 
		hr_analyzer->local_min_val) / hr_analyzer->hysteresis_div;
}

void hr_analyzer_reset(hr_analyzer_st *hr_analyzer)
{
	hr_analyzer_reset_local_max_min(hr_analyzer);

	hr_analyzer->prev_sample_val = 0;
	hr_analyzer->hysteresis = 0;
	hr_analyzer->prev_beat_ts = 0;
	hr_analyzer->heart_rate_val = 0.0f;
}

hr_analyzer_st *hr_analyzer_init(int32_t hysteresis_div)
{
	hr_analyzer_st *hr_analyzer = malloc(sizeof(hr_analyzer_st));
	if (hr_analyzer == NULL) return NULL;

	if (hysteresis_div == 0) hysteresis_div = 5;

	hr_analyzer_st hr_analyzer_buf = {
		.heart_rate_max_val = HR_MAX_RES_VAL,
		.heart_rate_min_val = HR_MIN_RES_VAL,
		.hysteresis_div = hysteresis_div
	};

	memcpy(hr_analyzer, &hr_analyzer_buf, sizeof(hr_analyzer_st));

	hr_analyzer_reset(hr_analyzer);

	return hr_analyzer;
}

void hr_analyzer_deinit(hr_analyzer_st *hr_analyzer)
{
	if (hr_analyzer == NULL) return;

	free(hr_analyzer);
}

static inline void hr_analyzer_find_local_max(hr_analyzer_st *hr_analyzer, int32_t new_sample_val)
{
	switch (hr_analyzer->local_max_state)
	{
		case LOCAL_EXTM_IDLE:
			if (hr_analyzer->local_min_state == LOCAL_EXTM_FOUND)
			{
				hr_analyzer->local_max_val = new_sample_val;
				hr_analyzer->local_max_state = LOCAL_EXTM_STARTED;
			}
			break;
		case LOCAL_EXTM_STARTED:
			if (new_sample_val <= (hr_analyzer->local_max_val - hr_analyzer->hysteresis))
			{
				hr_analyzer->local_max_state = LOCAL_EXTM_FOUND;
			}
		default:
			if (new_sample_val > hr_analyzer->local_max_val)
			{
				hr_analyzer->local_max_val = new_sample_val;
			}
			break;
	}
}

static inline void hr_analyzer_find_local_min(hr_analyzer_st *hr_analyzer, int32_t new_sample_val)
{
	switch (hr_analyzer->local_min_state)
	{
		case LOCAL_EXTM_IDLE:
			hr_analyzer->local_min_val = new_sample_val;
			hr_analyzer->local_min_state = LOCAL_EXTM_STARTED;
			break;
		case LOCAL_EXTM_STARTED:
			if (new_sample_val >= (hr_analyzer->local_min_val + hr_analyzer->hysteresis))
			{
				hr_analyzer->local_min_state = LOCAL_EXTM_FOUND;
			}
		default:
			if (new_sample_val < hr_analyzer->local_min_val)
			{
				hr_analyzer->local_min_val = new_sample_val;
			}
			break;
	}
}

static inline bool hr_analyzer_find_beat_threshold_crossing(hr_analyzer_st *hr_analyzer, int32_t new_sample_val)
{
	if (hr_analyzer->local_max_state == LOCAL_EXTM_FOUND && 
		hr_analyzer->local_min_state == LOCAL_EXTM_FOUND)
	{
		hr_analyzer->beat_threshold = (hr_analyzer->local_max_val + 
				hr_analyzer->local_min_val) / 2;

		if (new_sample_val < hr_analyzer->beat_threshold && 
				hr_analyzer->prev_sample_val >= hr_analyzer->beat_threshold)
		{
			return true;
		}
	}
	return false;
}

static inline float hr_analyzer_get_hr(hr_analyzer_st *hr_analyzer, uint32_t new_beat_ts)
{
	if (hr_analyzer->prev_beat_ts == 0)
	{
		hr_analyzer->prev_beat_ts = new_beat_ts;
		return HR_ANALYZER_EMPTY;
	}

	int32_t period_ms = new_beat_ts - hr_analyzer->prev_beat_ts;

	hr_analyzer->prev_beat_ts = new_beat_ts;

	float calc_heart_rate_val = (float) MS_PER_MINUTE / period_ms; // bpm

	if (calc_heart_rate_val <= hr_analyzer->heart_rate_max_val &&
			calc_heart_rate_val >= hr_analyzer->heart_rate_min_val)
	hr_analyzer->heart_rate_val = calc_heart_rate_val;

	return hr_analyzer->heart_rate_val;
}

bool hr_analyzer_process_sample(hr_analyzer_st *hr_analyzer, float *hr_val, 
		int32_t new_sample_val, uint32_t current_time_ms)
{
	if (hr_analyzer == NULL)
	{
		*hr_val = HR_ANALYZER_ERRROR;
		return false;
	}

	hr_analyzer_find_local_min(hr_analyzer, new_sample_val);
	hr_analyzer_find_local_max(hr_analyzer, new_sample_val);

	if (hr_analyzer_find_beat_threshold_crossing(hr_analyzer, new_sample_val))
	{
		hr_analyzer_set_new_hysteresis(hr_analyzer);
		hr_analyzer_reset_local_max_min(hr_analyzer);

		*hr_val = hr_analyzer_get_hr(hr_analyzer, current_time_ms);
		return true;
	}

	hr_analyzer->prev_sample_val = new_sample_val;

	if (hr_analyzer->prev_beat_ts && current_time_ms - hr_analyzer->prev_beat_ts > RESET_ANALYZER_TMO_MS)
	{
		hr_analyzer_reset_local_max_min(hr_analyzer);
		hr_analyzer->hysteresis = 0;
		hr_analyzer->prev_beat_ts = 0;

		*hr_val = HR_ANALYZER_EMPTY;
		return true;
	}

	*hr_val = hr_analyzer->heart_rate_val;
	return false;
}
