#pragma once

#include "../state.hpp"
#include "ThreeWire.h"
#include "RtcDS1302.h"


//======= Fonctions de ce segment=======

//Donne la premiere valeur de l'heure
void initHeure();
//modifie la date
RtcDateTime modifierDate();
//modifie l'heure
RtcDateTime modifierHeure();
//Permet de créer deux textes a partir de la date et l'heure
void texteDateHeure();
//verifie que la date est valide
bool TestValidite();
//boucle de ce segment
void loopRtc();