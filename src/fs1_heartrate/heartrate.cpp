#include "heartrate.h"
#include "../config.h"
#include <string.h>

// ======================== Internal state ======================== //

// Short moving average //
static u16 smoothBuffer[PPG_SMOOTH_WINDOW_SAMPLES];
static u8 smoothIndex;
static u8 smoothCount;
static u32 smoothSum;

// Baseline moving average //
static u16 baselineBuffer[PPG_BASELINE_WINDOW_SAMPLES];
static u16 baselineIndex;
static u16 baselineCount;
static u32 baselineSum;

// Normalization window //
static u16 normalizeBuffer[PPG_NORMALIZE_WINDOW_SAMPLES];
static u8 normalizeIndex;
static u8 normalizeCount;

// Beat detection //
static bool pulseActive;
static u32 lastBeatTime;

// IBI averaging //
static u16 ibiBuffer[BPM_IBI_AVERAGE_COUNT];
static u8 ibiIndex;
static u8 ibiCount;


// ======================== Internal functions ======================== //

static u16 apply_moving_average(u16 sample) {
	smoothSum -= smoothBuffer[smoothIndex];

	smoothBuffer[smoothIndex] = sample;
	smoothSum += sample;

	smoothIndex++;

	if (smoothIndex >= PPG_SMOOTH_WINDOW_SAMPLES) { smoothIndex = 0; }
	if (smoothCount < PPG_SMOOTH_WINDOW_SAMPLES) { smoothCount++; }

	return (u16)(smoothSum / smoothCount);
}


static u16 estimate_baseline(u16 sample) {
	baselineSum -= baselineBuffer[baselineIndex];

	baselineBuffer[baselineIndex] = sample;
	baselineSum += sample;

	baselineIndex++;

	if (baselineIndex >= PPG_BASELINE_WINDOW_SAMPLES) { baselineIndex = 0; }
	if (baselineCount < PPG_BASELINE_WINDOW_SAMPLES) { baselineCount++; }

	return (u16)(baselineSum / baselineCount);
}


static i16 center_signal(u16 sample, u16 baseline) {
	return (i16)sample - (i16)baseline;
}


static f32 normalize_signal(i16 sample) {
	u16 magnitude = (sample < 0) ? (u16)(-sample) : (u16)sample;

	normalizeBuffer[normalizeIndex] = magnitude;
	normalizeIndex++;

	if (normalizeIndex >= PPG_NORMALIZE_WINDOW_SAMPLES) { normalizeIndex = 0; }
	if (normalizeCount < PPG_NORMALIZE_WINDOW_SAMPLES) { normalizeCount++; }

	u16 maximum = 0;

	for (u8 i = 0; i < normalizeCount; i++) {
		if (normalizeBuffer[i] > maximum) { maximum = normalizeBuffer[i]; }
	}

	if (maximum == 0) { return 0.0f; }

	return (f32)sample / (f32)maximum;
}


static bool detect_beat(f32 sample, u32 timestampMs) {
	u32 elapsed = timestampMs - lastBeatTime;

	if (!pulseActive) {
		if (sample >= PPG_BEAT_THRESHOLD && elapsed >= PPG_REFRACTORY_MS) {
			pulseActive = true;
			return true;
		}
	}
	else if (sample < PPG_BEAT_THRESHOLD) { pulseActive = false; }

	return false;
}


static u8 compute_bpm(u16 ibi) {
	ibiBuffer[ibiIndex] = ibi;

	ibiIndex++;

	if (ibiIndex >= BPM_IBI_AVERAGE_COUNT) { ibiIndex = 0; }
	if (ibiCount < BPM_IBI_AVERAGE_COUNT) { ibiCount++; }

	u32 total = 0;

	for (u8 i = 0; i < ibiCount; i++) { total += ibiBuffer[i]; }

	u16 averageIbi = (u16)(total / ibiCount);
	if (averageIbi == 0) { return 0; }

	u32 bpm = 60000UL / averageIbi;
	if (bpm > 255u) { bpm = 255u; }

	return (u8)bpm;
}


// ======================== Public API ======================== //

void heartrate_init(void) {
    memset(smoothBuffer, 0, sizeof(smoothBuffer));
    memset(baselineBuffer, 0, sizeof(baselineBuffer));
    memset(normalizeBuffer, 0, sizeof(normalizeBuffer));
    memset(ibiBuffer, 0, sizeof(ibiBuffer));
	smoothIndex = 0;
	smoothCount = 0;
	smoothSum = 0;

	baselineIndex = 0;
	baselineCount = 0;
	baselineSum = 0;

	normalizeIndex = 0;
	normalizeCount = 0;

	pulseActive = false;
	lastBeatTime = 0;

	ibiIndex = 0;
	ibiCount = 0;
}


void heartrate_update(CoreState *state, u16 rawSample, u32 timestampMs) {
	
    u16 smoothed = apply_moving_average(rawSample);
	u16 baseline = estimate_baseline(smoothed);
	i16 centered = center_signal(smoothed, baseline);
	f32 normalized = normalize_signal(centered);

	state->ppgValue = normalized;

	if (detect_beat(normalized, timestampMs)) {
		if (lastBeatTime != 0) {
			u32 ibi32 = timestampMs - lastBeatTime;
			if (ibi32 <= UINT16_MAX) {
				u16 ibi = (u16)ibi32;
				u8 bpm = compute_bpm(ibi);

				if (bpm >= BPM_MIN_VALID && bpm <= BPM_MAX_VALID) {
					state->bpm = bpm;
					state->bpmValid = true;
				} else { state->bpmValid = false; }
			}
		}
		lastBeatTime = timestampMs;
	}

	if (lastBeatTime != 0 && timestampMs - lastBeatTime >= PPG_NO_BEAT_TIMEOUT_MS) {
		state->bpm = 0;
		state->bpmValid = false;

		pulseActive = false;
		lastBeatTime = 0;

		ibiIndex = 0;
		ibiCount = 0;
	}
}
