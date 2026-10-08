#include <Arduino.h>

#include "storage.hpp"
#include <EEPROM.h>

namespace Storage {

namespace {

constexpr uint8_t MAGIC_0 = 0xC6;
constexpr uint8_t MAGIC_1 = 0xEC;

constexpr uint16_t DATA_START = HEADER_SIZE;

// Bit 38 = 1, bit 39 = 0
constexpr uint8_t VALID_MASK = 0xC0;
constexpr uint8_t VALID_VALUE = 0x40;

uint8_t nextSlot = 0;
uint8_t recordCount = 0;
bool initialized = false;

uint16_t addressOf(uint8_t slot) {
    return DATA_START + uint16_t(slot) * RECORD_SIZE;
}

bool isLeapYear(uint16_t year) {
    return (year % 4 == 0) &&
           ((year % 100 != 0) || (year % 400 == 0));
}

bool validRecord(const Record& r) {
    if (r.year < BASE_YEAR || r.year > BASE_YEAR + 31)
        return false;

    uint16_t maxDay = isLeapYear(r.year) ? 365 : 364;

    return r.dayOfYear <= maxDay &&
           r.minuteOfDay < 1440 &&
           r.bpm > 0;
}

// 40 bits :
// [39:38] signature 01
// [37:33] reserves
// [32:28] annee
// [27:19] jour
// [18:8]  minute
// [7:0]   BPM

uint64_t pack(const Record& r) {
    uint64_t bits = r.bpm;

    bits |= uint64_t(r.minuteOfDay) << 8;
    bits |= uint64_t(r.dayOfYear) << 19;
    bits |= uint64_t(r.year - BASE_YEAR) << 28;

    bits |= uint64_t(1) << 38;

    return bits;
}

Record unpack(uint64_t bits) {
    Record r;

    r.bpm = bits & 0xFF;
    r.minuteOfDay = (bits >> 8) & 0x7FF;
    r.dayOfYear = (bits >> 19) & 0x1FF;
    r.year = BASE_YEAR + ((bits >> 28) & 0x1F);

    return r;
}

void writeRecord(uint8_t slot, const Record& r) {
    uint16_t addr = addressOf(slot);
    uint64_t bits = pack(r);

    // Invalider avant de modifier les donnees.
    uint8_t last = EEPROM.read(addr + 4);
    EEPROM.update(addr + 4, last & ~VALID_MASK);

    // Ecriture des quatre premiers octets.
    for (uint8_t i = 0; i < 4; ++i) {
        EEPROM.update(
            addr + i,
            uint8_t(bits >> (8 * i))
        );
    }

    // Dernier octet : signature ecrite en dernier.
    EEPROM.update(addr + 4, uint8_t(bits >> 32));
}

bool readRecord(uint8_t slot, Record& r) {
    uint16_t addr = addressOf(slot);

    uint8_t last = EEPROM.read(addr + 4);

    if ((last & VALID_MASK) != VALID_VALUE)
        return false;

    uint64_t bits = 0;

    for (uint8_t i = 0; i < RECORD_SIZE; ++i) {
        bits |= uint64_t(EEPROM.read(addr + i))
                << (8 * i);
    }

    r = unpack(bits);

    return validRecord(r);
}

void saveHeader() {
    EEPROM.update(2, nextSlot);
    EEPROM.update(3, recordCount);
}

} // namespace

void init() {
    initialized = false;

    bool headerValid =
        EEPROM.read(0) == MAGIC_0 &&
        EEPROM.read(1) == MAGIC_1;

    if (headerValid) {
        nextSlot = EEPROM.read(2);
        recordCount = EEPROM.read(3);

        headerValid =
            nextSlot < CAPACITY &&
            recordCount <= CAPACITY;
    }

    if (!headerValid) {
        clear();
        return;
    }

    initialized = true;
}

bool save(const Record& record) {
    if (!initialized || !validRecord(record))
        return false;

    writeRecord(nextSlot, record);

    nextSlot = (nextSlot + 1) % CAPACITY;

    if (recordCount < CAPACITY)
        ++recordCount;

    saveHeader();

    return true;
}

bool read(uint8_t index, Record& record) {
    if (!initialized || index >= recordCount)
        return false;

    uint16_t slot =
        (uint16_t(nextSlot) + CAPACITY - 1 - index)
        % CAPACITY;

    return readRecord(uint8_t(slot), record);
}

uint8_t count() {
    return initialized ? recordCount : 0;
}

uint8_t capacity() {
    return CAPACITY;
}

void clear() {
    // Invalidation logique des enregistrements.
    for (uint8_t i = 0; i < CAPACITY; ++i) {
        uint16_t addr = addressOf(i) + 4;

        EEPROM.update(
            addr,
            EEPROM.read(addr) & ~VALID_MASK
        );
    }

    nextSlot = 0;
    recordCount = 0;

    // Metadonnees d'abord, signature globale ensuite.
    saveHeader();

    EEPROM.update(0, MAGIC_0);
    EEPROM.update(1, MAGIC_1);

    initialized = true;
}

} // namespace Storage
