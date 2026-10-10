#include "Encodeur.hpp"
#include "config.h"

Encodeur* Encodeur::instance = nullptr;

Encodeur::Encodeur()
    : encodeur(PIN_ENCODEUR_A, PIN_ENCODEUR_B, PIN_ENCODEUR_BOUTON) {
    this->etat = INACTIF;
    transitionsEnAttente = 0;
    Encodeur::instance = this;

    // Recuperer les transitions unitaires : le regroupement est fait ici.
    encodeur.useQuadPrecision(true);
    encodeur.setClickHandler(appui);
    encodeur.setEncoderHandler(rotation);
}

void Encodeur::loopEncodeur() {
    this->etat = INACTIF;
    encodeur.update();

    // Priorite au clic ; les crans en attente seront servis au tour suivant.
    if (this->etat == PULL) return;

    if (transitionsEnAttente >= TRANSITIONS_PAR_CRAN) {
        transitionsEnAttente -= TRANSITIONS_PAR_CRAN;
        this->etat = DROITE;
    } else if (transitionsEnAttente <= -TRANSITIONS_PAR_CRAN) {
        transitionsEnAttente += TRANSITIONS_PAR_CRAN;
        this->etat = GAUCHE;
    }
}

void Encodeur::appui(EncoderButton&) {
    instance->etat = PULL;
}

void Encodeur::rotation(EncoderButton& eb) {
    // Conserver le signe ET la magnitude (increment() renvoie un int16_t).
    // Une transition aller puis retour s'annule avant de produire un cran.
    instance->transitionsEnAttente += eb.increment();
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