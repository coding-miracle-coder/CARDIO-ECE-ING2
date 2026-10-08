
#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace Storage {

constexpr uint16_t BASE_YEAR = 2026;
constexpr uint8_t RECORD_SIZE = 5;
constexpr uint8_t HEADER_SIZE = 4;

constexpr uint8_t CAPACITY = 204;

struct Record {
    uint16_t year;
    uint16_t dayOfYear;    // 0 = 1er janvier
    uint16_t minuteOfDay;  // 0 = 00:00
    uint8_t bpm;
};

void init();

bool save(const Record& record);

// index 0 = enregistrement le plus récent
bool read(uint8_t index, Record& record);

uint8_t count();
uint8_t capacity();

void clear();

}
