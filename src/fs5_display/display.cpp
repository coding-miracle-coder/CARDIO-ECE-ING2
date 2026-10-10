#include "display.hpp"

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <string.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#include "config.h"
#include "Encodeur.hpp"

//Appel du mosule RTC de fs2
extern RtcDS1302<ThreeWire> Rtc;
extern Encodeur temp;

// Un seul buffer de page de 128 octets, partage entre les deux OLED.
static U8G2_SSD1306_128X64_NONAME_1_HW_I2C ecran(U8G2_R0, U8X8_PIN_NONE);
static bool displayActif = false;

enum EcranEnCours : u8 { AUCUN, PRINCIPAL, PPG };
static EcranEnCours ecranEnCours = AUCUN;

// Base de temps du graphique, calculee depuis les acquisitions.
static u32 resteTempsPPG = 0;
static bool premierPointPPG = true;

//Reglages du graphique

#define GRAPH_X             18u
#define GRAPH_Y_HAUT        12u
#define GRAPH_Y_BAS         50u
#define GRAPH_LARGEUR       (DISPLAY_WIDTH - GRAPH_X)


// 4 a 40 quarts de seconde = 1,00 a 10,00 secondes.
static u8 baseTempsQuarts = 20; // Demarrage a 5,00 s.

// Le PPG est deja normalise entre environ -1 et +1 dans heartrate.cpp
// Stockage entre -100 et +100 pour economiser la RAM
static i8 historiquePPG[GRAPH_LARGEUR];

static u32 dernierPointPPG = 0;
static u32 dernierAffichagePPG = 0;
static u32 dernierAffichagePrincipal = 0;

// Les donnees d'une image restent identiques pendant ses huit pages.
// L'acquisition continue d'alimenter historiquePPG entre les pages.
// Une seule image etant dessinee a la fois, les instantanes partagent la RAM.
static union {
    struct {
        char heure[9];
        u8 bpm;
        bool bpmValid;
    } principal;
    struct {
        i8 valeurs[GRAPH_LARGEUR];
        u8 baseQuarts;
    } ppg;
} image;

static bool adressePresente(u8 adresse) {
    Wire.beginTransmission(adresse);
    return Wire.endTransmission() == 0;
}

static void viderHistoriquePPG() {
    for (u8 i = 0; i < GRAPH_LARGEUR; i++) {
        historiquePPG[i] = 0;
    }
}

static i8 convertirPPG(f32 valeur) {
    if (valeur > 1.0f) {
        valeur = 1.0f;
    }
    else if (valeur < -1.0f) {
        valeur = -1.0f;
    }

    return (i8)(valeur * 100.0f);
}

static u8 convertirY(i8 valeur) {
    i16 milieu = (GRAPH_Y_HAUT + GRAPH_Y_BAS) / 2;
    i16 amplitude = (GRAPH_Y_BAS - GRAPH_Y_HAUT) / 2;

    i16 y = milieu - ((i16)valeur * amplitude) / 100;

    if (y < GRAPH_Y_HAUT) {
        y = GRAPH_Y_HAUT;
    }
    else if (y > GRAPH_Y_BAS) {
        y = GRAPH_Y_BAS;
    }

    return (u8)y;
}

static void ajouterPointPPG(f32 valeur) {
    // Decale les anciennes valeurs vers la gauche
    for (u8 i = 0; i < GRAPH_LARGEUR - 1u; i++) {
        historiquePPG[i] = historiquePPG[i + 1u];
    }

    historiquePPG[GRAPH_LARGEUR - 1u] = convertirPPG(valeur);
}

static void gererEncodeur() {
    if (temp.aDroite() && baseTempsQuarts < 40u) {
        ++baseTempsQuarts;
    }
    else if (temp.aGauche() && baseTempsQuarts > 4u) {
        --baseTempsQuarts;
    }
    else {
        return;
    }

    resetDisplaySignal();
}

static void afficherEcranPrincipal() {
    // Les coordonnees verticales U8g2 sont ici des lignes de base.
    ecran.setFont(u8g2_font_logisoso16_tn);
    ecran.setCursor(16, 22);
    ecran.print(image.principal.heure);
    ecran.drawLine(8, 27, 119, 27);

    ecran.setFont(u8g2_font_6x10_tr);
    ecran.setCursor(8, 46);
    ecran.print(F("BPM"));

    ecran.setFont(u8g2_font_logisoso24_tn);
    ecran.setCursor(45, 59);
    if (image.principal.bpmValid) {
        ecran.print(image.principal.bpm);
    } else {
        // Independamment des glyphes disponibles dans la police numerique.
        ecran.drawHLine(47, 46, 16);
    }
}

static void afficherEcranPPG() {
    ecran.setFont(u8g2_font_6x10_tr);

    // Titre
    ecran.setCursor(0, 8);
    ecran.print(F("PPG"));

    // Graduation verticale avec norma du signal
    ecran.setCursor(0, GRAPH_Y_HAUT + 6u);
    ecran.print(F("1"));

    ecran.setCursor(0, ((GRAPH_Y_HAUT + GRAPH_Y_BAS) / 2u) + 3u);
    ecran.print(F("0"));

    ecran.setCursor(0, GRAPH_Y_BAS);
    ecran.print(F("-1"));

    // Axes et ligne du zero
    ecran.drawLine(GRAPH_X, GRAPH_Y_HAUT, GRAPH_X, GRAPH_Y_BAS);
    ecran.drawLine(GRAPH_X, GRAPH_Y_BAS, DISPLAY_WIDTH - 1u, GRAPH_Y_BAS);
    ecran.drawLine(GRAPH_X, (GRAPH_Y_HAUT + GRAPH_Y_BAS) / 2u,
                   DISPLAY_WIDTH - 1u, (GRAPH_Y_HAUT + GRAPH_Y_BAS) / 2u);

    // Graduations horizontales tous les quarts de l'ecran
    for (u8 i = 0; i <= 4; i++) {
        u8 x = GRAPH_X + ((GRAPH_LARGEUR - 1u) * i) / 4u;
        ecran.drawLine(x, GRAPH_Y_BAS, x, GRAPH_Y_BAS + 2u);
    }

    // Courbe PPG
    for (u8 i = 1; i < GRAPH_LARGEUR; i++) {
        u8 x1 = GRAPH_X + i - 1u;
        u8 x2 = GRAPH_X + i;
        u8 y1 = convertirY(image.ppg.valeurs[i - 1u]);
        u8 y2 = convertirY(image.ppg.valeurs[i]);

        ecran.drawLine(x1, y1, x2, y2);
    }

    // Base de temps
    ecran.setCursor(18, 63);
    ecran.print(F("T="));

    ecran.print(image.ppg.baseQuarts / 4u);
    ecran.print('.');

    const u8 centiemes = (image.ppg.baseQuarts % 4u) * 25u;
    if (centiemes < 10u) ecran.print('0');
    ecran.print(centiemes);

    ecran.print(F("s/"));
    ecran.print(GRAPH_LARGEUR - 1u);
    ecran.print(F("px"));

}

// Une seule page dessinee/transmise par appel : retour rapide a l'acquisition.
static void continuerAffichage() {
    if (ecranEnCours == PRINCIPAL) {
        afficherEcranPrincipal();
    } else {
        afficherEcranPPG();
    }

    if (!ecran.nextPage()) {
        ecranEnCours = AUCUN;
    }

    #if defined(WIRE_HAS_TIMEOUT)
        if (Wire.getWireTimeoutFlag()) {
            Wire.clearWireTimeoutFlag();
            displayActif = false;
            ecranEnCours = AUCUN;
            Serial.println(F("OLED: timeout I2C"));
        }
    #endif
}

//Fonctions publiques
void initDisplay() {
    displayActif = false;
    ecranEnCours = AUCUN;

    Wire.begin();
    #if defined(WIRE_HAS_TIMEOUT)
        Wire.setWireTimeout(3000, true);
        Wire.clearWireTimeoutFlag();
    #endif

    // begin() U8g2 ne teste pas la presence physique des ecrans.
    if (!adressePresente(DISPLAY1_I2C_ADDRESS) ||
        !adressePresente(DISPLAY2_I2C_ADDRESS)) {
        Serial.println(F("OLED: ecran absent a 0x3C ou 0x3D"));
        return;
    }

    ecran.setBusClock(400000UL);
    // U8g2 attend l'adresse I2C decalee d'un bit.
    ecran.setI2CAddress(DISPLAY1_I2C_ADDRESS << 1);
    ecran.begin();
    ecran.setI2CAddress(DISPLAY2_I2C_ADDRESS << 1);
    ecran.begin();
    ecran.setFontPosBaseline();

    #if defined(WIRE_HAS_TIMEOUT)
        if (Wire.getWireTimeoutFlag()) {
            Wire.clearWireTimeoutFlag();
            Serial.println(F("OLED: timeout initialisation"));
            return;
        }
    #endif

    displayActif = true;
    resetDisplaySignal();
    dernierAffichagePPG = millis() - 100UL;
    dernierAffichagePrincipal = millis() - 1000UL;
    Serial.println(F("OLED: 0x3C + 0x3D, U8g2 pages"));
}

// Appelee pour chaque echantillon traite par FS1.
// timestampMs est l'instant d'acquisition, pas l'instant d'affichage.
void displaySample(f32 valeur, u32 timestampMs) {
    if (premierPointPPG) {
        premierPointPPG = false;
        dernierPointPPG = timestampMs;
        ajouterPointPPG(valeur);
        return;
    }

    u32 ecoule = timestampMs - dernierPointPPG;
    dernierPointPPG = timestampMs;

    // On conserve le reste de la division pour eviter la derive.
    resteTempsPPG += ecoule * (GRAPH_LARGEUR - 1u);

    u32 duree = (u32)baseTempsQuarts * 250UL;

    if (resteTempsPPG >= duree) {
        resteTempsPPG %= duree;
        ajouterPointPPG(valeur);
    }
}

// Utilisee notamment apres une perte d'echantillons.
void resetDisplaySignal() {
    viderHistoriquePPG();
    premierPointPPG = true;
    resteTempsPPG = 0;
}

void loopDisplay(CoreState *state) {
    gererEncodeur();

    if (!displayActif || state == NULL) {
        return;
    }

    if (ecranEnCours != AUCUN) {
        continuerAffichage();
        return;
    }

    const u32 maintenant = millis();
    if (maintenant - dernierAffichagePrincipal >= 1000UL) {
        const RtcDateTime date = Rtc.GetDateTime();
        if (date.IsValid()) {
            snprintf(image.principal.heure, sizeof(image.principal.heure),
                     "%02u:%02u:%02u", (unsigned)date.Hour(),
                     (unsigned)date.Minute(), (unsigned)date.Second());
        } else {
            strcpy(image.principal.heure, "--:--:--");
        }
        image.principal.bpm = state->bpm;
        image.principal.bpmValid = state->bpmValid;
        ecran.setI2CAddress(DISPLAY1_I2C_ADDRESS << 1);
        ecranEnCours = PRINCIPAL;
        dernierAffichagePrincipal = maintenant;
    } else if (maintenant - dernierAffichagePPG >= 100UL) {
        memcpy(image.ppg.valeurs, historiquePPG, sizeof(historiquePPG));
        image.ppg.baseQuarts = baseTempsQuarts;
        ecran.setI2CAddress(DISPLAY2_I2C_ADDRESS << 1);
        ecranEnCours = PPG;
        dernierAffichagePPG = maintenant;
    } else {
        return;
    }

    ecran.firstPage();
    continuerAffichage();
}
