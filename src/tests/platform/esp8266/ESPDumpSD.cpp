#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

namespace {
    void listDir(const String &dirname, int levels = 0) {
        Serial.printf("DIR: %s\n", dirname.c_str());

        File root = SD.open(dirname.c_str());
        if (!root) {
            Serial.println("  Failed to open directory");
            return;
        }

        if (!root.isDirectory()) {
            Serial.println("  Not a directory");
            return;
        }

        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                Serial.print("  [DIR]  ");
                Serial.println(file.name());

                if (levels > 0) {
                    String childPath = dirname;
                    if (!childPath.endsWith("/")) {
                        childPath += "/";
                    }
                    childPath += file.name();
                    listDir(childPath, levels - 1);
                }
            } else {
                Serial.print("  [FILE] ");
                Serial.print(file.name());
                Serial.print("  (");
                Serial.print(file.size());
                Serial.println(" bytes)");
            }

            file = root.openNextFile();
        }
    }

} // namespace

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=== SD Filesystem Dump ===");

    SPI.begin();

    if (!SD.begin(D1)) {   // CS pin
        Serial.println("SD init FAILED");
        return;
    }

    Serial.println("SD init OK\n");

    listDir("/", 2);  // dump root (2 levels deep)
}

void loop() {
    // nothing
}