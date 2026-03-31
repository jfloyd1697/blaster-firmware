// main.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include "core/Blaster.h"
#include "core/weapons/WeaponJSONLoader.h"
#include "../include/platform/pc/PCPlatform.h"


int main() {
    // Initialize platform services (audio, input, time, debug)
    auto services = PCPlatformFactory::create();

    // Load weapon profiles
    std::vector<SoundBank> banks = loadSoundBanks(services.assetRoot + "weapon_profiles.json");
    if (banks.empty()) {
        std::cerr << "No weapons found!" << std::endl;
        return 1;
    }

    // Create the Blaster
    Blaster blaster(services, banks);

    std::cout << "Blaster PC Demo - Space: Fire | Left: Prev Weapon | Right: Next Weapon | ESC: Quit\n" << std::endl;

    bool running = true;

    while (running) {
        running = blaster.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // main loop
    }
    return 0;
}

