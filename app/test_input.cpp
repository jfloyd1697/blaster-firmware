#include <iostream>
#include <thread>

#include "WeaponJSONLoader.h"
#include "../core/Blaster.h"
#include "../platform/pc/PCPlatform.h"  // contains PCPlatformFactory

int main() {
    try {
        // --- Create all platform services ---
        PlatformServices platform = PCPlatformFactory::create();

        // --- Construct the Blaster with fully initialized services ---
        Blaster blaster(platform, loadWeaponProfiles(platform.assetRoot + "weapon_profiles.json"));

        std::cout << "Blaster initialized successfully!\n";

        // --- Example loop for testing ---
        while (true) {
            // Safety: check input service before reading
            if (!platform.input) {
                std::cerr << "Input service not available!\n";
                std::cin.get();
                break;
            }

            // Example: check trigger
            if (platform.input->isTriggerPressed()) {
                blaster.fire();
            }

            // Sleep to simulate frame time
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}