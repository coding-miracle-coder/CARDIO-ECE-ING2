#include "Encodeur.hpp"
#include "config.h"

Encodeur* Encodeur::instance = nullptr;

Encodeur::Encodeur()
    : encodeur(PIN_ENCODEUR_A, PIN_ENCODEUR_B, PIN_ENCODEUR_BOUTON) {
    this->etat = INACTIF;
    Encodeur::instance = this;

    encodeur.setClickHandler(appui);
    encodeur.setEncoderHandler(rotation);
}

void Encodeur::loopEncodeur() {
    this->etat = INACTIF;
    encodeur.update();
}

void Encodeur::appui(EncoderButton&) {
    instance->etat = PULL;
}

void Encodeur::rotation(EncoderButton& eb) {
    i8 buffer = eb.increment();

    if (buffer > 0) {
        instance->etat = DROITE;
    } else if (buffer < 0) {
        instance->etat = GAUCHE;
    }
}

bool Encodeur::aAppui() {
    return this->etat == PULL;
}

bool Encodeur::aDroite() {
    return this->etat == DROITE;
}

bool Encodeur::aGauche() {
    return this->etat == GAUCHE;
}

bool Encodeur::inactif() {
    return this->etat == INACTIF;
}