#include <Arduino.h>
#include <Wire.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "config.h"

// Objet deja defini dans rtc.cpp.
extern RtcDS1302<ThreeWire> Rtc;

static const uint8_t boutons[] = {
    PIN_BOUTON_ENREGISTRER,
    PIN_BOUTON_SON,
    PIN_BOUTON_RETOUR,
    PIN_ENCODEUR_BOUTON
};

static uint8_t derniereLecture[4];
static uint8_t etatStable[4];
static uint32_t dernierChangement[4] = {};

static void nomBouton(uint8_t index) {
    switch (index) {
        case 0: Serial.print(F("ENREGISTRER")); break;
        case 1: Serial.print(F("SON"));         break;
        case 2: Serial.print(F("RETOUR"));      break;
        case 3: Serial.print(F("CLIC ENCODEUR")); break;
    }
}

static void scanI2C() {
    Serial.println(F("=== SCAN I2C ==="));
    uint8_t trouves = 0;

    for (uint8_t adresse = 1; adresse < 127; ++adresse) {
        Wire.beginTransmission(adresse);

        if (Wire.endTransmission() == 0) {
            Serial.print(F("Trouve : 0x"));
            if (adresse < 16) Serial.print('0');
            Serial.println(adresse, HEX);
            ++trouves;
        }
    }

    Serial.print(F("Nombre de peripheriques : "));
    Serial.println(trouves);
    Serial.println(F("Attendu : 0x3C et 0x3D"));
}

static void afficherHeure() {
    if (!Rtc.IsDateTimeValid()) {
        Serial.println(F("RTC : date invalide / liaison a verifier"));
        return;
    }

    RtcDateTime date = Rtc.GetDateTime();

    if (!date.IsValid()) {
        Serial.println(F("RTC : lecture invalide"));
        return;
    }

    Serial.print(F("RTC : "));
    Serial.print(date.Year());
    Serial.print('-');
    Serial.print(date.Month());
    Serial.print('-');
    Serial.print(date.Day());
    Serial.print(' ');

    if (date.Hour() < 10) Serial.print('0');
    Serial.print(date.Hour());
    Serial.print(':');
    if (date.Minute() < 10) Serial.print('0');
    Serial.print(date.Minute());
    Serial.print(':');
    if (date.Second() < 10) Serial.print('0');
    Serial.print(date.Second());

    if (!Rtc.GetIsRunning()) {
        Serial.print(F(" [HORLOGE ARRETEE]"));
    }

    Serial.println();
}

void setup() {
    Serial.begin(115200);

    const uint8_t leds[] = {
        PIN_ROUGE, PIN_VERTE, PIN_JAUNE
    };

    for (uint8_t i = 0; i < 3; ++i) {
        digitalWrite(leds[i], LOW);
        pinMode(leds[i], OUTPUT);
    }

    digitalWrite(PIN_BUZZER, LOW);
    pinMode(PIN_BUZZER, OUTPUT);

    pinMode(PIN_ENCODEUR_A, INPUT_PULLUP);
    pinMode(PIN_ENCODEUR_B, INPUT_PULLUP);

    for (uint8_t i = 0; i < 4; ++i) {
        pinMode(boutons[i], INPUT_PULLUP);
        derniereLecture[i] = digitalRead(boutons[i]);
        etatStable[i] = derniereLecture[i];
    }

    delay(500);
    Serial.println(F("=== TEST CABLAGE ==="));

    // Delais volontaires : test de demarrage uniquement.
    for (uint8_t i = 0; i < 3; ++i) {
        digitalWrite(leds[i], HIGH);
        delay(350);
        digitalWrite(leds[i], LOW);
        delay(150);
    }

    tone(PIN_BUZZER, 1000, 150);

    Wire.begin();

#if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(3000, true);
#endif

    scanI2C();

    Rtc.Begin(); // Pas de SetDateTime : preserve l'heure existante.
    afficherHeure();

    Serial.println(F("Appuie sur les boutons et tourne doucement l'encodeur."));
}

void loop() {
    const uint32_t maintenant = millis();

    // Boutons : changement accepte apres 25 ms de stabilite.
    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t lecture = digitalRead(boutons[i]);

        if (lecture != derniereLecture[i]) {
            derniereLecture[i] = lecture;
            dernierChangement[i] = maintenant;
        }

        if (lecture != etatStable[i] &&
            maintenant - dernierChangement[i] >= 25) {
            etatStable[i] = lecture;

            nomBouton(i);
            Serial.println(
                lecture == LOW ? F(" : APPUYE") : F(" : RELACHE")
            );

            if (i == 1 && lecture == LOW) {
                tone(PIN_BUZZER, 1000, 100);
            }
        }
    }

    // Diagnostic brut des deux contacts, pas un decodeur de crans.
    static uint8_t ancienAB = 0xFF;

    const uint8_t ab =
        (digitalRead(PIN_ENCODEUR_A) << 1) |
         digitalRead(PIN_ENCODEUR_B);

    if (ab != ancienAB && Serial.availableForWrite() >= 24) {
        ancienAB = ab;
        Serial.print(F("ENC A="));
        Serial.print((ab >> 1) & 1);
        Serial.print(F(" B="));
        Serial.println(ab & 1);
    }

    static uint32_t derniereHeure = 0;

    if (maintenant - derniereHeure >= 1000) {
        derniereHeure = maintenant;
        afficherHeure();
    }
}