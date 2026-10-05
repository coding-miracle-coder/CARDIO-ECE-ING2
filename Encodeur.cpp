#include "Encodeur.h"

    Encodeur::Encodeur()
        : encodeur(pinA,pinB,pinBouton){

        this->etat = INACTIF;
        Encodeur::instance = this;

        encodeur.setClickHandler(appui);
        encodeur.setEncoderHandler(rotation);
    }
    
    action Encodeur::quelleAction(){
        encodeur.update();

        action ancienneAction = this->etat;
        this->etat = INACTIF;

        return ancienneAction;
    }


    void Encodeur::appui(EncoderButton&){
        instance->etat = PULL;
    }
    
    void Encodeur::rotation(EncoderButton& eb){
        if (eb.increment() > 0){
            instance->etat = DROITE;
        }
        else if (eb.increment() < 0){
            instance->etat = GAUCHE ;
        }

    }



