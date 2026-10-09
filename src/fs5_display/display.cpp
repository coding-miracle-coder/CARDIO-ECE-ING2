#include "display.hpp"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#include "config.h"
#include "Encodeur.hpp"

//Appel du mosule RTC de fs2
extern RtcDS1302<ThreeWire> Rtc;
extern Encodeur temp;

// Un seul objet Adafruit_SSD1306/buffeur utiliser pour les deux ecrans à cause de la RAM
static Adafruit_SSD1306 ecran(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, -1);

static bool displayActif = false;

// Un seul transfert en cours avec le buffer partage.
static bool transfertEnCours = false;
static u8 adresseTransfert = 0;
static uint16_t positionTransfert = 0;

// Base de temps du graphique, calculee depuis les acquisitions.
static u32 resteTempsPPG = 0;
static bool premierPointPPG = true;

//Reglages du graphique

#define GRAPH_X             18u
#define GRAPH_Y_HAUT        12u
#define GRAPH_Y_BAS         50u
#define GRAPH_LARGEUR       (DISPLAY_WIDTH - GRAPH_X)

// Bases de temps possibles
// L'encodeur permet de passer de l'une a l'autre
static const u8 basesTempsSecondes[] = {1, 2, 5, 10};
static u8 indiceBaseTemps = 2; // Demarrage sur 5 secondes

// Le PPG est deja normalise entre environ -1 et +1 dans heartrate.cpp
// Stockage entre -100 et +100 pour economiser la RAM
static i8 historiquePPG[GRAPH_LARGEUR];

static u32 dernierPointPPG = 0;
static u32 dernierAffichagePPG = 0;
static u32 dernierAffichagePrincipal = 0;

// Fonctions internes

// Envoie le buffer Adafruit vers l'adresse I2C choisie
// UN SEUL buffer pour les deux OLED
static void envoyerBuffer(u8 adresse) {
    Wire.beginTransmission(adresse);
    Wire.write(0x00);
    Wire.write(SSD1306_PAGEADDR);
    Wire.write(0);
    Wire.write((DISPLAY_HEIGHT / 8u) - 1u);
    Wire.write(SSD1306_COLUMNADDR);
    Wire.write(0);
    Wire.write(DISPLAY_WIDTH - 1u);

    if (Wire.endTransmission() != 0) {
        displayActif = false;
        return;
    }

    adresseTransfert = adresse;
    positionTransfert = 0;
    transfertEnCours = true;
}

static void continuerEnvoiBuffer() {
    if (!transfertEnCours) {
        return;
    }

    uint8_t *buffer = ecran.getBuffer();
    const uint16_t tailleBuffer =
        (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8u;

    Wire.beginTransmission(adresseTransfert);
    Wire.write(0x40);

    for (u8 i = 0; i < 16 && positionTransfert < tailleBuffer; i++) {
        Wire.write(buffer[positionTransfert]);
        positionTransfert++;
    }

    if (Wire.endTransmission() != 0) {
        transfertEnCours = false;
        displayActif = false;
        return;
    }

    if (positionTransfert >= tailleBuffer) {
        transfertEnCours = false;
    }
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
    temp.loopEncodeur();

    if (temp.aDroite()) {
        if (indiceBaseTemps < 3u) {
            indiceBaseTemps++;
            viderHistoriquePPG();
            premierPointPPG = true;
            resteTempsPPG = 0;
        }
    }
    else if (temp.aGauche()) {
        if (indiceBaseTemps > 0u) {
            indiceBaseTemps--;
            viderHistoriquePPG();
            premierPointPPG = true;
            resteTempsPPG = 0;
        }
    }
}

static void afficherEcranPrincipal(CoreState *state) {
    RtcDateTime maintenant = Rtc.GetDateTime();

    ecran.clearDisplay();
    ecran.setTextColor(SSD1306_WHITE);

    //Heure
    ecran.setTextSize(2);
    ecran.setCursor(16, 4);

    if (maintenant.IsValid()) {
        if (maintenant.Hour() < 10) ecran.print('0');
        ecran.print(maintenant.Hour());
        ecran.print(':');

        if (maintenant.Minute() < 10) ecran.print('0');
        ecran.print(maintenant.Minute());
        ecran.print(':');

        if (maintenant.Second() < 10) ecran.print('0');
        ecran.print(maintenant.Second());
    }
    else {
        ecran.print("--:--:--");
    }

    ecran.drawLine(8, 27, 119, 27, SSD1306_WHITE);

    //BPM
    ecran.setTextSize(1);
    ecran.setCursor(8, 38);
    ecran.print("BPM");

    ecran.setTextSize(3);
    ecran.setCursor(45, 34);

    if (state->bpmValid) {
        ecran.print(state->bpm);
    }
    else {
        //un tiret si la valeur BPM n'est pas valide
        ecran.print('-');
    }

    envoyerBuffer(DISPLAY1_I2C_ADDRESS);
}

static void afficherEcranPPG() {
    ecran.clearDisplay();
    ecran.setTextColor(SSD1306_WHITE);
    ecran.setTextSize(1);

    // Titre
    ecran.setCursor(0, 0);
    ecran.print("PPG");

    // Graduation verticale avec norma du signal
    ecran.setCursor(0, GRAPH_Y_HAUT - 3u);
    ecran.print("1");

    ecran.setCursor(0, ((GRAPH_Y_HAUT + GRAPH_Y_BAS) / 2u) - 3u);
    ecran.print("0");

    ecran.setCursor(0, GRAPH_Y_BAS - 5u);
    ecran.print("-1");

    // Axes et ligne du zero
    ecran.drawLine(GRAPH_X, GRAPH_Y_HAUT, GRAPH_X, GRAPH_Y_BAS, SSD1306_WHITE);
    ecran.drawLine(GRAPH_X, GRAPH_Y_BAS, DISPLAY_WIDTH - 1u, GRAPH_Y_BAS, SSD1306_WHITE);
    ecran.drawLine(GRAPH_X, (GRAPH_Y_HAUT + GRAPH_Y_BAS) / 2u,
                   DISPLAY_WIDTH - 1u, (GRAPH_Y_HAUT + GRAPH_Y_BAS) / 2u,
                   SSD1306_WHITE);

    // Graduations horizontales tous les quarts de l'ecran
    for (u8 i = 0; i <= 4; i++) {
        u8 x = GRAPH_X + ((GRAPH_LARGEUR - 1u) * i) / 4u;
        ecran.drawLine(x, GRAPH_Y_BAS, x, GRAPH_Y_BAS + 2u, SSD1306_WHITE);
    }

    // Courbe PPG
    for (u8 i = 1; i < GRAPH_LARGEUR; i++) {
        u8 x1 = GRAPH_X + i - 1u;
        u8 x2 = GRAPH_X + i;
        u8 y1 = convertirY(historiquePPG[i - 1u]);
        u8 y2 = convertirY(historiquePPG[i]);

        ecran.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    }

    // Base de temps
    ecran.setCursor(18, 55);
    ecran.print("T=");
    ecran.print(basesTempsSecondes[indiceBaseTemps]);
    ecran.print("s/");
    ecran.print(GRAPH_LARGEUR - 1u);
    ecran.print("px");

    envoyerBuffer(DISPLAY2_I2C_ADDRESS);
}

//Fonctions publiques
void initDisplay() {
    displayActif = false;
    transfertEnCours = false;

    Wire.begin();
    #if defined(WIRE_HAS_TIMEOUT)
        Wire.setWireTimeout(3000, true);
    #endif

    if (!ecran.begin(SSD1306_SWITCHCAPVCC, DISPLAY1_I2C_ADDRESS)) {
        return;
    }

    if (!ecran.begin(SSD1306_SWITCHCAPVCC, DISPLAY2_I2C_ADDRESS)) {
        return;
    }

    Wire.setClock(400000);



    displayActif = true;
    viderHistoriquePPG();
    ecran.clearDisplay();

    envoyerBuffer(DISPLAY1_I2C_ADDRESS);
    while (transfertEnCours) {
        continuerEnvoiBuffer();
    }

    if (!displayActif) {
        return;
    }

    envoyerBuffer(DISPLAY2_I2C_ADDRESS);
    while (transfertEnCours) {
        continuerEnvoiBuffer();
    }

    if (!displayActif) {
        return;
    }

    premierPointPPG = true;
    resteTempsPPG = 0;

    dernierAffichagePPG = millis();
    dernierAffichagePrincipal = millis() - 1000UL;
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

    u32 duree =
        (u32)basesTempsSecondes[indiceBaseTemps] * 1000UL;

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

    if (transfertEnCours) {
        continuerEnvoiBuffer();
        return;
    }

    // Le buffer est maintenant libre : on peut dessiner dedans.
    u32 maintenant = millis();

    if (maintenant - dernierAffichagePrincipal >= 1000UL) {
        afficherEcranPrincipal(state);
        dernierAffichagePrincipal = maintenant;
        return;
    }

    if (maintenant - dernierAffichagePPG >= 100UL) {
        afficherEcranPPG();
        dernierAffichagePPG = maintenant;
    }
}