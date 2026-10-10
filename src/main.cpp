#include <Arduino.h>

#include "config.h"
#include "state.hpp"
#include "Encodeur.hpp"
#include "acquisition/acquisition.hpp"
#include "fs1_heartrate/heartrate.h"
#include "fs2_rtc/rtc.hpp"
#include "fs5_display/display.hpp"

extern Encodeur temp;

static CoreState state = {
    0.0f,
    0,
    false,
    HEALTH_UNKNOWN
};

static u16 dernierBrut = 0;
static u16 pertesObservees = 0;
static u32 dernierDiagnostic = 0;

void setup() {
    Serial.begin(115200);

    pinMode(PIN_VERTE, OUTPUT);
    pinMode(PIN_JAUNE, OUTPUT);
    pinMode(PIN_ROUGE, OUTPUT);
    digitalWrite(PIN_VERTE, LOW);
    digitalWrite(PIN_JAUNE, LOW);
    digitalWrite(PIN_ROUGE, LOW);

    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    pinMode(PIN_BOUTON_ENREGISTRER, INPUT_PULLUP);
    pinMode(PIN_BOUTON_SON, INPUT_PULLUP);
    pinMode(PIN_BOUTON_RETOUR, INPUT_PULLUP);

    initHeure();
    heartrate_init();
    initDisplay();

    Serial.println(F("Integration acquisition + FS1 + FS5 : 500 Hz"));

    // Demarrer apres les initialisations potentiellement longues.
    Acquisition::begin();
}

void loop() {
    const u16 pertes = Acquisition::dropped();

    if (pertes != pertesObservees) {
        pertesObservees = pertes;

        // Une discontinuite invalide l'historique du traitement.
        Acquisition::discardPending();
        heartrate_init();
        resetDisplaySignal();

        state.ppgValue = 0.0f;
        state.bpm = 0;
        state.bpmValid = false;
        state.healthState = HEALTH_UNKNOWN;
    }

    Acquisition::Sample sample;

    // Priorite au traitement de tous les echantillons disponibles.
    while (Acquisition::pop(sample)) {
        dernierBrut = sample.raw;

        heartrate_update(
            &state,
            sample.raw,
            sample.timestampMs
        );
        displaySample(state.ppgValue, sample.timestampMs);
    }

    temp.loopEncodeur();
    loopRtc();
    loopDisplay(&state);

    // Diagnostic limite a 5 lignes par seconde.
    const u32 maintenant = millis();

    if (maintenant - dernierDiagnostic >= 200UL &&
        Serial.availableForWrite() >= 60) {

        dernierDiagnostic = maintenant;

        Serial.print(F("RAW="));
        Serial.print(dernierBrut);

        Serial.print(F(" PPG="));
        Serial.print(state.ppgValue, 3);

        Serial.print(F(" BPM="));
        if (state.bpmValid) {
            Serial.print(state.bpm);
        } else {
            Serial.print('-');
        }

        Serial.print(F(" pertes="));
        Serial.println(pertesObservees);
    }
}