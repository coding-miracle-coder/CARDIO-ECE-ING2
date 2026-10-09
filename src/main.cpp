#include <Arduino.h>
#include "config.h"

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

void setup(){

    
    initPins();         // Initialise les pins

}

void loop(){}