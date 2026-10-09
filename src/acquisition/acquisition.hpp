#pragma once
#include <stdint.h>

namespace Acquisition {
struct Sample { uint16_t raw; uint32_t timestampMs; };
// Owns Timer1 and ADC (A0). Do not use analogRead, Servo or PWM D9/D10
// while running. Digital input D9 and digital/tone output D10 remain usable.
void begin();
bool pop(Sample& sample);
uint16_t dropped(); // Saturating count: queue full or ADC still busy.
void discardPending();
}
