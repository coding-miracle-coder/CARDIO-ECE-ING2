#include <Arduino.h>
#include <EEPROM.h>

struct TestData {
    uint16_t bpm;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

constexpr int TEST_ADDRESS = 900;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("=== EEPROM UNIT TEST ==="));

    TestData expected = {72, 8, 10, 2026};
    TestData actual = {};

    // Write only bytes that have changed
    EEPROM.put(TEST_ADDRESS, expected);

    // Read the data back
    EEPROM.get(TEST_ADDRESS, actual);

    bool passed =
        actual.bpm == expected.bpm &&
        actual.day == expected.day &&
        actual.month == expected.month &&
        actual.year == expected.year;

    Serial.print(F("BPM: "));
    Serial.println(actual.bpm);

    Serial.print(F("Date: "));
    Serial.print(actual.day);
    Serial.print('/');
    Serial.print(actual.month);
    Serial.print('/');
    Serial.println(actual.year);

    Serial.println(
        passed ? F("TEST PASSED") : F("TEST FAILED")
    );
}

void loop() {}
