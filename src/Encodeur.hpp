#pragma once

#include "state.hpp"
#include "types.h"
#include <EncoderButton.h>

typedef enum action{
    INACTIF,
    GAUCHE,
    DROITE,
    PULL
}action;

class Encodeur{
private : 
    EncoderButton encodeur;
    action etat;
    // Transitions signees non encore converties en crans.
    i32 transitionsEnAttente;
    static constexpr i32 TRANSITIONS_PAR_CRAN = 2;

    static Encodeur *instance;
    static void appui (EncoderButton& eb);
    static void rotation (EncoderButton& eb);
    
public:
    Encodeur();
    void loopEncodeur();
    bool aGauche();
    bool aDroite();
    bool aAppui();
    bool inactif();
};
