#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("=== A0 Ladder Calibration ===");
    Serial.println("Press each button and note values");
}

void loop() {
    int value = analogRead(A0);

    Serial.print("A0 = ");
    Serial.println(value);

    delay(100);
}