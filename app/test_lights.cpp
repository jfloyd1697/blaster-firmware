#include <iostream>

#include "DemoApp.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "lights/DebugLights.h"
#include "lights/LightEngine.h"
#include "PCPlatform.h"
#include <SDL.h>


DemoApp::DemoApp() {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
    window_ = SDL_CreateWindow("Lights Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400, 0);
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);

    // Create lights engine
    leds_ = std::make_shared<Lights::LedLights>(10);
    auto debug = std::make_shared<PCDebug>();
    auto debugLights = std::make_shared<Lights::DebugLights>(leds_, debug);

    lights_ = std::make_shared<Lights::LightEngine>(debugLights.get());

}

DemoApp::~DemoApp() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void DemoApp::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            running_ = false;
        }
    }
}

void DemoApp::renderLEDs() {
    const auto& leds = leds_->getLEDs();
    int numLEDs = leds.size();
    int cx = 300, cy = 200, radius = 100;

    for (int i = 0; i < numLEDs; ++i) {
        float angle = (2.0f * 3.1415926f * i) / numLEDs;
        int x = static_cast<int>(cx + radius * cos(angle));
        int y = static_cast<int>(cy + radius * sin(angle));

        SDL_SetRenderDrawColor(renderer_, leds[i].r, leds[i].g, leds[i].b, 255);
        SDL_Rect rect{x-10, y-10, 20, 20};
        SDL_RenderFillRect(renderer_, &rect);
    }
}

void DemoApp::render() {
    SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
    SDL_RenderClear(renderer_);

    // Step animations
    lights_->step();

    // Render LED ring
    renderLEDs();

    // ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug");
    ImGui::Text("Lights Demo");
    ImGui::Text("Number of LEDs: %zu", leds_->getLEDs().size());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);

    SDL_RenderPresent(renderer_);
}

void DemoApp::run() {
    while (running_) {
        handleEvents();
        render();
        SDL_Delay(200); // Slow down for demo
    }
}

int main(int argc, char** argv) {
    DemoApp app = DemoApp();
    app.run();
    return 0;
}