#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

#define DISPLAY1_I2C_ADDRESS 0x3C
#define DISPLAY2_I2C_ADDRESS 0x3D

// ================== 𝐒𝐭𝐚𝐧𝐝𝐚𝐫𝐝 𝐏𝐚𝐫𝐚𝐦𝐞𝐭𝐞𝐫𝐬 ================== //

/* PPG */
#define PPG_SAMPLE_RATE_HZ              500u
#define BPM_IBI_AVERAGE_COUNT           10u
#define PPG_SMOOTH_WINDOW_SAMPLES       8u
#define PPG_BASELINE_WINDOW_SAMPLES     128u

/* PPG processing */
#define PPG_NORMALIZE_WINDOW_SAMPLES    64u

#define PPG_BEAT_THRESHOLD              0.50f

#define PPG_REFRACTORY_MS               250u
#define PPG_NO_BEAT_TIMEOUT_MS          2500u


/* Displays */
#define DISPLAY_WIDTH                   128u
#define DISPLAY_HEIGHT                  64u

/* Storage */
#define STORAGE_AVERAGE_COUNT           10u
#define MAX_SAVED_READINGS              20u // À calculer

/* Physical / validity limits */
#define BPM_MIN_VALID                   30u
#define BPM_MAX_VALID                   220u
#define BPM_TRIGGER_1                   1u // À changer
#define BPM_TRIGGER_2                   1u // À changer

// ================== Pins — Arduino Nano ATmega328P ================== //

// Capteur PPG
#define PIN_PPG                 A0

// LEDs indépendantes — actives à HIGH
#define PIN_ROUGE               A1
#define PIN_VERTE               A2
#define PIN_JAUNE               A3

// OLED : bus I2C matériel commun
// SDA = A4 ; SCL = A5
// Adresses déjà définies : 0x3C et 0x3D

// Encodeur numérique           // 𝐃𝐨𝐧𝐞
#define PIN_ENCODEUR_B          2
#define PIN_ENCODEUR_BOUTON     3
#define PIN_ENCODEUR_A          4

// Boutons                       // 𝐃𝐨𝐧𝐞
#define PIN_BOUTON_ENREGISTRER   5
#define PIN_BOUTON_SON           6
#define PIN_BOUTON_RETOUR        7

// RTC DS1302                   // 𝐃𝐨𝐧𝐞
#define PIN_IO                  8
#define PIN_CE                  9
#define PIN_CLK                 10

// Buzzer
#define PIN_BUZZER              11

// D0 / D1 : réservées à la liaison série
// D11 / D12 : libres
// A6 / A7 : entrées analogiques uniquement

#endif