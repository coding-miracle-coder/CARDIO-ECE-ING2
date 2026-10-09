
#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

// DS1302 : DAT = D4, CLK = D5, RST = D6
ThreeWire wire(4, 5, 6);
RtcDS1302<ThreeWire> rtc(wire);

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("=== DS1302 UNIT TEST ==="));

    rtc.Begin();

    if (rtc.GetIsWriteProtected()) {
        rtc.SetIsWriteProtected(false);
    }

    if (!rtc.IsDateTimeValid()) {
        Serial.println(F("RTC invalid: setting compile time"));
        rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
    }

    if (!rtc.GetIsRunning()) {
        rtc.SetIsRunning(true);
    }

    Serial.println(F("RTC initialized"));
}

void loop() {
    static uint32_t lastRead = 0;

    if (millis() - lastRead < 1000UL) {
        return;
    }

    lastRead = millis();

    RtcDateTime now = rtc.GetDateTime();

    Serial.print(now.Day());
    Serial.print('/');
    Serial.print(now.Month());
    Serial.print('/');
    Serial.print(now.Year());

    Serial.print(' ');

    if (now.Hour() < 10) Serial.print('0');
    Serial.print(now.Hour());
    Serial.print(':');

    if (now.Minute() < 10) Serial.print('0');
    Serial.print(now.Minute());
    Serial.print(':');

    if (now.Second() < 10) Serial.print('0');
    Serial.println(now.Second());
}
