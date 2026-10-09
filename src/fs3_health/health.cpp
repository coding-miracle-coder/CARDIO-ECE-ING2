#include "health.hpp"
HealthState etat = HEALTH_UNKNOWN;


void DefinirEtat(u8 bpm){

    if (bpm<SEUIL_BAS){
        etat = HEALTH_UNKNOWN;
    }
    else if (bpm<SEUIL_MID){
        etat = HEALTH_LOW;
    }
    else if (bpm<SEUIL_HAUT){
        etat = HEALTH_NORMAL;
    }
    else if (bpm<SEUIL_CRITIQUE){
        etat = HEALTH_HIGH;
    }
    else if (bpm>SEUIL_CRITIQUE){
        etat = HEALTH_UNKNOWN;
    }

}

String TextePatient(){
    switch (etat){
        case HEALTH_UNKNOWN:
            return "BPM NON IDENTIFIABLE";
            break;
        case HEALTH_LOW:
            return "BPM FAIBLE";
            break;
        case HEALTH_NORMAL:
            return "BPM NORMAL";
            break;
        case HEALTH_HIGH:
            return "BPM HAUT";
            break;

    }
}

void AllumerLed(LedFlag couleur){
    digitalWrite(PIN_ROUGE,couleur & LED_RED);
    digitalWrite(PIN_VERTE,couleur &  LED_GREEN);
    digitalWrite(PIN_JAUNE,couleur & LED_YELLOW);
}

LedFlag ChoixCouleur(){
    switch (etat){
        case HEALTH_UNKNOWN :
            return LED_OFF;
            break;
        case HEALTH_LOW :
            return LED_YELLOW;
            break;
        case HEALTH_NORMAL : 
            return LED_GREEN;
            break;
        case HEALTH_HIGH :
            return LED_RED;
            break;
    }
}


void loopHealth(){
    AllumerLed(ChoixCouleur());
}

//fonctionAffichage y aura tout l'affichage oled a gerer ici juset pour cette partie: 
void AfficherHealthOled(u8 bpm){
    
}