#pragma once

#include "../config.h"
#include "../state.hpp"

#define SEUIL_BAS 30
#define SEUIL_MID 80
#define SEUIL_HAUT 120
#define SEUIL_CRITIQUE 150

//Flags pour LED
typedef uint8_t LedFlag;

#define LED_OFF ((LedFlag)0)
#define LED_RED ((LedFlag)1u<<0)
#define LED_GREEN ((LedFlag)1u<<1)
#define LED_YELLOW ((LedFlag)1u<<2)

void DefinirEtat(u8 bpm);
String TextePatient();
LedFlag ChoixCouleur();
void AllumerLed(LedFlag couleur);
void loopHealth();