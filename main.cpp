#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

#include "MeasurementsLog.cpp"
#include "Forest.cpp"
#include "GUI.cpp"


int main(int, char **) {
    int lastHeight = currentHeight, lastWidth = currentWidth, lastSize = currentSize;
    auto start = std::chrono::high_resolution_clock::now();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    createWindow();

    if (renderer == nullptr) {
        SDL_Log("Error creating SDL_Renderer!");
        return EXIT_FAILURE;
    }

    initForest();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Main loop
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                stopSimulation();
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                stopSimulation();
            if (event.type == SDL_MOUSEBUTTONDOWN)
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int x, y;
                    SDL_GetMouseState(&x, &y);

                    int gridX = x / currentSize;
                    int gridY = y / currentSize;

                    if (gridX >= 0 && gridX < currentWidth && gridY >= 0 && gridY < currentHeight) {
                        if (forest[gridX][gridY] == TREE) {
                            forest[gridX][gridY] = FIRE;
                        }
                    }
                }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        mainMenu();
        initSettings(lastHeight, lastWidth, lastSize);

        if ((currentStep == maxSteps) && startMeasure) {
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

            measurements.AddLog("[%s] Time taken for %i steps: %lld ms\n", "info", maxSteps, duration.count());

            if (!measureSteps.empty()) {
                nextSteps();
                start = std::chrono::high_resolution_clock::now();
            } else {
                resetMeasure();
                measurements.AddLog("[%s] Measurement finished!\n", "info");
            }
        }

        if (measurementWindow) {
            ImGui::SetNextWindowSize(ImVec2(400, 250));

            ImGui::Begin("Measurements", &measurementWindow);

            if (!startMeasure) {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 100, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 90, 0, 255));
                if (ImGui::SmallButton("Start")) {
                    if (!startMeasure) {
                        start = std::chrono::high_resolution_clock::now();
                        measureSteps = std::priority_queue<int, std::vector<int>, std::greater<>>(num_steps,
                                                                                                  num_steps +
                                                                                                  sizeof(num_steps) /
                                                                                                  sizeof(num_steps[0]));
                        startMeasure = true;
                        nextSteps();
                        measurements.AddLog("[%s] Measurement started!\n", "info");
                    } else {
                        measurements.AddLog("[%s] Measurement already started!\n", "warn");
                    }
                }
                ImGui::PopStyleColor(3);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(100, 0, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(90, 0, 0, 255));
                if (ImGui::SmallButton("Stop")) {
                    resetMeasure();
                    measurements.AddLog("[%s] Measurement aborted!\n", "warn");
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();

                ImGui::ProgressBar(progressCurrentStep / progressAllSteps, ImVec2(0.0f, 0.0f));
            }

            ImGui::End();
            measurements.Draw("Measurements", &measurementWindow);
        }

        stepForest(fire, growth);

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8) (clearColor.x * 255), (Uint8) (clearColor.y * 255),
                               (Uint8) (clearColor.z * 255), (Uint8) (clearColor.w * 255));
        SDL_RenderClear(renderer);

        for (int i = 0; i < currentWidth; ++i) {
            for (int j = 0; j < currentHeight; ++j) {
                if (forest[i][j] == TREE) {
                    drawSquare(i, j, {0, 128, 0, 255}); // Green for tree
                } else if (forest[i][j] == FIRE) {
                    drawSquare(i, j, {200, 0, 0, 255}); // Red for fire
                }
            }
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);

        if (startMeasure) {
            currentStep++;
            progressCurrentStep++;
        }
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}