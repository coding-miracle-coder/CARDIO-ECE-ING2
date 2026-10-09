
#include <Arduino.h>
#include "fs6_storage/storage.hpp"

void printRecords() {
    Serial.print(F("Records: "));
    Serial.println(Storage::count());

    for (uint8_t i = 0; i < Storage::count(); ++i) {
        Storage::Record r;

        if (!Storage::read(i, r)) {
            Serial.println(F("Invalid record"));
            continue;
        }

        Serial.print(F("BPM: "));
        Serial.print(r.bpm);

        Serial.print(F(" | Year: "));
        Serial.print(r.year);

        Serial.print(F(" | Day: "));
        Serial.print(r.dayOfYear);

        Serial.print(F(" | Minute: "));
        Serial.println(r.minuteOfDay);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Storage::init();

    Serial.println(F("=== FS6 EEPROM TEST ==="));
    printRecords();

    Serial.println(F("s = save, r = read, c = clear"));
}

void loop() {
    if (!Serial.available())
        return;

    char command = Serial.read();

    if (command == 's') {
        Storage::Record r = {
            2026,
            280,
            720,
            72
        };

        Serial.println(
            Storage::save(r)
            ? F("SAVE OK")
            : F("SAVE FAILED")
        );
    }

    if (command == 'r')
        printRecords();

    if (command == 'c') {
        Storage::clear();
        Serial.println(F("EEPROM CLEARED"));
    }
}
