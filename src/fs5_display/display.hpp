#pragma once
#include "state.hpp"

// Initialise les deux ecrans OLED
void initDisplay();
void displaySample(f32 valeur, u32 timestampMs);
void resetDisplaySignal();

// Boucle d'affichage; Elle affiche l'heure + BPM sur l'ecran 1,
// le PPG sur l'ecran 2 et utilise l'evenement encodeur pour la base de temps.
// Appeler temp.loopEncodeur() une seule fois dans main, avant loopDisplay().
void loopDisplay(CoreState *state);
