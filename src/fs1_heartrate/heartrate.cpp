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
static u16 amplitudeReference;
static u32 dernierDecay;
static bool detectionArmee;

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


// Estimation de la moyenne
static u16 estimate_baseline(u16 sample) {
	baselineSum -= baselineBuffer[baselineIndex];		// On prend la somme, on retire le plus vieux élément

	baselineBuffer[baselineIndex] = sample;				// On ajoute le nouvel élément
	baselineSum += sample;								// On ajoute le dernier élément à la somme

	baselineIndex++;									// Avance l'index

	if (baselineIndex >= PPG_BASELINE_WINDOW_SAMPLES) { baselineIndex = 0; }
	if (baselineCount < PPG_BASELINE_WINDOW_SAMPLES) { baselineCount++; }

	return (u16)(baselineSum / baselineCount);			// Calcule la moyenne
}


static i16 center_signal(u16 sample, u16 baseline) {
	return (i16)sample - (i16)baseline;
}

static void update_amplitude(i16 sample, u32 timestampMs) {
    const u16 magnitude =
        (sample < 0) ? (u16)(-sample) : (u16)sample;

    // Descente lente de l'enveloppe, sans tableau supplementaire.
    const u32 pas =
        (timestampMs - dernierDecay) / PPG_ENVELOPE_DECAY_MS;

    if (pas > 0) {
        dernierDecay += pas * PPG_ENVELOPE_DECAY_MS;

        if (pas >= amplitudeReference) {
            amplitudeReference = 0;
        } else {
            amplitudeReference -= (u16)pas;
        }
    }

    if (magnitude > amplitudeReference) {
        amplitudeReference = magnitude;
    }
}

static f32 normalize_signal(i16 sample) {
    if (amplitudeReference < PPG_MIN_AMPLITUDE_ADC) {
        return 0.0f;
    }

    return (f32)sample / (f32)amplitudeReference;
}

static bool detect_beat(i16 sample, u32 timestampMs) {
    if (baselineCount < PPG_BASELINE_WINDOW_SAMPLES ||
        amplitudeReference < PPG_MIN_AMPLITUDE_ADC) {
        detectionArmee = false;
        pulseActive = false;
        return false;
    }

    const i16 seuilHaut = (i16)(amplitudeReference / 2u);

    // Rearmement seulement apres retour a zero ou en dessous.
    // Cela evite les declenchements repetes autour du seuil haut.
    if (sample <= 0) {
        detectionArmee = true;
        pulseActive = false;
        return false;
    }

    if (!detectionArmee || sample < seuilHaut) {
        return false;
    }

    // Consommer cette montee, meme si elle arrive trop tot.
    detectionArmee = false;

    const u32 intervalleMinimal =
        (60000UL + BPM_MAX_VALID - 1u) / BPM_MAX_VALID;

    u32 attente = PPG_REFRACTORY_MS;

    if (attente < intervalleMinimal) {
        attente = intervalleMinimal;
    }

    if (lastBeatTime != 0 &&
        timestampMs - lastBeatTime < attente) {
        return false;
    }

    pulseActive = true;
    return true;
}

static void reset_beats(CoreState *state) {
    pulseActive = false;
    detectionArmee = false;
    lastBeatTime = 0;

    // compute_bpm() ne lit que les ibiCount entrees valides.
    ibiIndex = 0;
    ibiCount = 0;

    state->bpm = 0;
    state->bpmValid = false;
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
    memset(ibiBuffer, 0, sizeof(ibiBuffer));
	smoothIndex = 0;
	smoothCount = 0;
	smoothSum = 0;

	baselineIndex = 0;
	baselineCount = 0;
	baselineSum = 0;

	amplitudeReference = 0;
	dernierDecay = 0;
	detectionArmee = false;

	pulseActive = false;
	lastBeatTime = 0;

	ibiIndex = 0;
	ibiCount = 0;
}

void heartrate_update(
    CoreState *state,
    u16 rawSample,
    u32 timestampMs
) {
    if (state == nullptr) {
        return;
    }

    const u16 smoothed = apply_moving_average(rawSample);
    const u16 baseline = estimate_baseline(smoothed);
    const i16 centered = center_signal(smoothed, baseline);

    update_amplitude(centered, timestampMs);
    state->ppgValue = normalize_signal(centered);

    // Verifier le timeout AVANT de traiter un nouveau battement.
    if (lastBeatTime != 0 &&
        timestampMs - lastBeatTime >= PPG_NO_BEAT_TIMEOUT_MS) {
        reset_beats(state);
    }

    if (baselineCount < PPG_BASELINE_WINDOW_SAMPLES ||
        amplitudeReference < PPG_MIN_AMPLITUDE_ADC) {
        reset_beats(state);
        return;
    }

    if (!detect_beat(centered, timestampMs)) {
        return;
    }

    // Le premier evenement donne seulement une origine temporelle.
    if (lastBeatTime == 0) {
        lastBeatTime = timestampMs;
        return;
    }

    const u32 ibi = timestampMs - lastBeatTime;

    const u32 ibiMin =
        (60000UL + BPM_MAX_VALID - 1u) / BPM_MAX_VALID;
    const u32 ibiMax = 60000UL / BPM_MIN_VALID;

    if (ibi < ibiMin) {
        return;
    }

    if (ibi > ibiMax) {
        reset_beats(state);
        lastBeatTime = timestampMs;
        return;
    }

    lastBeatTime = timestampMs;

    // Inserer uniquement un intervalle dans les bornes.
    const u8 bpm = compute_bpm((u16)ibi);

    // Attendre trois intervalles acceptes avant d'afficher un BPM.
    state->bpmValid =
        ibiCount >= 3u &&
        bpm >= BPM_MIN_VALID &&
        bpm <= BPM_MAX_VALID;

    state->bpm = state->bpmValid ? bpm : 0;
}