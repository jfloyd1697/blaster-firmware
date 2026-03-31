//
// Created by user on 3/27/2026.
//

#ifndef BLASTERFIRMWARE_DEMOAPP_H
#define BLASTERFIRMWARE_DEMOAPP_H

#pragma once
#include "lights/LightEngine.h"
#include <SDL.h>
#include <memory>

class DemoApp {
public:
    DemoApp();
    ~DemoApp();

    void run();

private:
    void handleEvents();
    void render();
    void renderLEDs() const;

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    bool running_ = true;

    std::shared_ptr<Lights::LightEngine> lights_;
    std::shared_ptr<Lights::LedLights> leds_;
};

#endif //BLASTERFIRMWARE_DEMOAPP_H