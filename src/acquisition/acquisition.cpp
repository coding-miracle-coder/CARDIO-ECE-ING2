#include "acquisition.hpp"
#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "../config.h"

#if !defined(__AVR_ATmega328P__)
#error This acquisition driver targets ATmega328P only.
#endif
static_assert(PIN_PPG == A0, "ADC channel is fixed to A0");
static_assert(PPG_SAMPLE_RATE_HZ == 500, "Timestamp step assumes 500 Hz");
static_assert(F_CPU == 16000000UL, "Timer configuration assumes 16 MHz");

namespace {
constexpr uint8_t SIZE = 16; // 15 usable entries = 30 ms
volatile Acquisition::Sample samples[SIZE];
volatile uint8_t head = 0, tail = 0;
volatile uint16_t losses = 0;
volatile uint32_t clockMs = 0, conversionMs = 0;
inline void countLoss() { if (losses != UINT16_MAX) ++losses; }
}

ISR(TIMER1_COMPA_vect) {
    clockMs += 2;
    if (ADCSRA & _BV(ADSC)) { countLoss(); return; }
    conversionMs = clockMs;
    ADCSRA |= _BV(ADSC);
}

ISR(ADC_vect) {
    const uint16_t raw = ADC;
    const uint8_t next = (head + 1) & (SIZE - 1);
    if (next == tail) { countLoss(); return; }
    samples[head].raw = raw;
    samples[head].timestampMs = conversionMs;
    head = next;
}

void Acquisition::begin() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        TCCR1A = 0;
        TCCR1B = 0;
        TIMSK1 = 0;
        head = tail = 0;
        losses = 0;
        clockMs = millis();
        ADMUX = _BV(REFS0); // AVcc reference, ADC0
        ADCSRB = 0;
        DIDR0 |= _BV(ADC0D);
        // 125 kHz ADC clock. Clear pending flag before enabling interrupt.
        ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADIF)
               | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
        TCNT1 = 0;
        OCR1A = F_CPU / 64UL / PPG_SAMPLE_RATE_HZ - 1;
        TIFR1 = _BV(OCF1A);
        TIMSK1 = _BV(OCIE1A);
        TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
    }
}

bool Acquisition::pop(Sample& sample) {
    bool available = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (tail != head) {
            sample.raw = samples[tail].raw;
            sample.timestampMs = samples[tail].timestampMs;
            tail = (tail + 1) & (SIZE - 1);
            available = true;
        }
    }
    return available;
}
uint16_t Acquisition::dropped() {
    uint16_t value;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = losses; }
    return value;
}
void Acquisition::discardPending() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { tail = head; }
}
