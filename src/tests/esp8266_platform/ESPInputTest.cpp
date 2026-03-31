#include <Arduino.h>

#include "../../../../lib/esp8266_platform/include/input/ESPInput.h"

namespace {
    ESPInput input(ESPInput::PinConfig{
        .trigger = D0,
        .ladder = A0,
        .quit = -1
    });
}

void setup() {


    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("=== ESPInput Test ===");
    Serial.println("Trigger: D0");
    Serial.println("Ladder: A0");
    Serial.println("Previous < 100");
    Serial.println("Next     < 300");
    Serial.println("Reload   < 800");
    Serial.println();

    input.begin();
    input.setLadderThresholds(100, 300, 800);
}

void loop() {
    input.update();

    if (input.wasTriggerPressed()) {
        Serial.println("Trigger pressed");
    }

    if (input.wasTriggerReleased()) {
        Serial.println("Trigger released");
    }

    if (input.wasNextShortPressed()) {
        Serial.println("Next short");
    }

    if (input.wasNextLongPressed()) {
        Serial.println("Next long");
    }

    if (input.wasPrevShortPressed()) {
        Serial.println("Previous short");
    }

    if (input.wasPrevLongPressed()) {
        Serial.println("Previous long");
    }

    if (input.wasReloadPressed()) {
        Serial.println("Reload pressed");
    }

    delay(5);
}