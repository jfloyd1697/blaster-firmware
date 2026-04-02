#include <Arduino.h>

#include "platform/esp8266/input/ESPInput.h"
#include "platform/esp8266/time/ESPTime.h"

namespace {
    auto esp_time = ESPTime();
    auto input = ESPInput(ESPInput::PinConfig{
        .trigger = D0,
        .ladder = A0,
        .quit = -1
    },
        &esp_time
    );

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