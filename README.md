# CARDIO ECE

Projet de cardio-fréquencemètre réalisé sur Arduino Uno dans le cadre du projet d'électronique ING2.

## Architecture

Le projet est organisé selon les fonctions secondaires du cahier des charges :

- `fs1_heartrate` : acquisition et traitement du signal PPG
- `fs2_rtc` : gestion de l'horloge temps réel
- `fs3_health` : classification LOW / NORMAL / HIGH
- `fs4_buzzer` : signal sonore synchronisé avec le rythme cardiaque
- `fs5_display` : affichage BPM / heure / PPG
- `fs6_storage` : enregistrement et consultation des mesures

Les paramètres globaux sont regroupés dans `config.h`.
Les types communs sont définis dans `types.h`.
L'état global du système est défini dans `state.hpp`.

## État actuel

FS1 est partiellement implémentée.

Le pipeline prévu est :

`ADC -> moyenne glissante -> baseline -> recentrage -> normalisation -> détection de battement -> IBI -> BPM`

Les constantes de traitement seront ajustées après acquisition du signal réel du capteur WPSE340.

FS2 est partiellement implémentée.

FS3 est partiellement implémentée.
Voir `TODO.md` pour le suivi détaillé.
