          #pragma once

#include "state.hpp"

// Initialise les deux ecrans OLED
void initDisplay();

// Boucle d'affichage; Elle affiche l'heure + BPM sur l'ecran 1,
// le PPG sur l'ecran 2 et lit l'encodeur pour changer la base de temps.
void loopDisplay(CoreState *state);