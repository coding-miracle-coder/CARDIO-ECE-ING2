
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

    Serial.println(F("=== EEPROM PERSISTENCE TEST ==="));

    TestData data = {};
    EEPROM.get(TEST_ADDRESS, data);

    Serial.print(F("BPM: "));
    Serial.println(data.bpm);

    Serial.print(F("Date: "));
    Serial.print(data.day);
    Serial.print('/');
    Serial.print(data.month);
    Serial.print('/');
    Serial.println(data.year);

    bool valid =
        data.bpm == 72 &&
        data.day == 8 &&
        data.month == 10 &&
        data.year == 2026;

    Serial.println(
        valid ? F("PERSISTENCE PASSED")
              : F("PERSISTENCE FAILED")
    );
}

void loop() {}
