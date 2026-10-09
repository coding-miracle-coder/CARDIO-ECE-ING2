#include <Arduino.h>
#include "config.h"
#include "state.hpp"
#include "acquisition/acquisition.hpp"
#include "fs1_heartrate/heartrate.h"
#include "fs2_rtc/rtc.hpp"
#include "fs5_display/display.hpp"

static void initPins() {
    pinMode(PIN_PPG, INPUT);

    // Précharger LOW avant d'activer les sorties.
    digitalWrite(PIN_ROUGE, LOW);
    digitalWrite(PIN_VERTE, LOW);
    digitalWrite(PIN_JAUNE, LOW);
    digitalWrite(PIN_BUZZER, LOW);

    pinMode(PIN_ROUGE, OUTPUT);
    pinMode(PIN_VERTE, OUTPUT);
    pinMode(PIN_JAUNE, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    pinMode(PIN_BOUTON_ENREGISTRER, INPUT_PULLUP);
    pinMode(PIN_BOUTON_SON, INPUT_PULLUP);
    pinMode(PIN_BOUTON_RETOUR, INPUT_PULLUP);

    // RTC, I2C et encodeur : initialisation par leurs pilotes.
}

static CoreState state = {0.0f, 0, false, HEALTH_UNKNOWN};
static uint16_t lastLosses = 0;

// Instantaneous stack/heap gap, NOT a stack high-water measurement.
extern char __heap_start;
extern char* __brkval;
static int freeRam() {
    char stack;
    return (int)(&stack - (__brkval ? __brkval : &__heap_start));
}

void setup() {
    initPins();
    Serial.begin(115200);
    initHeure();
    heartrate_init();
    initDisplay();
    Acquisition::begin(); // Start only after peripheral initialization.
}

void loop() {
    const uint16_t losses = Acquisition::dropped();
    if (losses != lastLosses) {
        lastLosses = losses;
        Acquisition::discardPending();
        heartrate_init();
        state = {0.0f, 0, false, HEALTH_UNKNOWN};
        resetDisplaySignal();
    }

    Acquisition::Sample sample;
    while (Acquisition::pop(sample)) {
        heartrate_update(&state, sample.raw, sample.timestampMs);
        displaySample(state.ppgValue, sample.timestampMs);
    }
    loopDisplay(&state);

    // Short diagnostics, once per second; no raw 500 Hz serial stream.
    static uint32_t lastReport = 0;
    const uint32_t now = millis();
    if (now - lastReport >= 1000 && Serial.availableForWrite() >= 48) {
        lastReport = now;
        Serial.print(F("BPM="));
        Serial.print(state.bpmValid ? state.bpm : 0);
        Serial.print(F(" dropped="));
        Serial.print(Acquisition::dropped());
        Serial.print(F(" free="));
        Serial.println(freeRam());
    }
}
