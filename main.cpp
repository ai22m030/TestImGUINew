#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

#include "MeasurementsLog.cpp"
#include "Forest.cpp"
#include "GUI.cpp"

int main(int, char **) {
    auto treeColor = DEFAULT_TREE_COLOR;
    auto fireColor = DEFAULT_FIRE_COLOR;
    auto calculateForest = true;
    auto lastHeight = currentHeight, lastWidth = currentWidth, lastSize = currentSize;
    auto start = std::chrono::high_resolution_clock::now();

    lastFrame = 0;
    lastUpdate = SDL_GetTicks();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Error: %s\n", SDL_GetError());
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
    unsigned int totalFrameTicks = 0;
    unsigned int totalFrames = 0;
    while (running) {
        totalFrames++;
        Uint32 startTicks = SDL_GetTicks();
        Uint64 startPerf = SDL_GetPerformanceCounter();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN)
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int x, y;
                    SDL_GetMouseState(&x, &y);

                    int gridX = x / currentSize;
                    int gridY = y / currentSize;

                    if (gridX >= 0 && gridX < currentWidth && gridY >= 0 && gridY < currentHeight)
                        if (forest[gridX][gridY] == TREE)
                            forest[gridX][gridY] = FIRE;
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
            ImGui::SetNextWindowSize(ImVec2(400, 300));

            ImGui::Begin("Measurements", &measurementWindow);

            if (!startMeasure) {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 100, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 90, 0, 255));

                if (ImGui::SmallButton("Start")) {
                    if (!startMeasure) {
                        start = std::chrono::high_resolution_clock::now();
                        measureSteps = std::priority_queue<int, std::vector<int>, std::greater<>>(numSteps,
                                                                                                  numSteps +
                                                                                                  sizeof(numSteps) /
                                                                                                  sizeof(numSteps[0]));
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

        if (colorWindow) {
            static ImVec4 pickerTreeColor{(float) treeColor.r, (float) treeColor.g, (float) treeColor.b,
                                          (float) treeColor.a};
            static ImVec4 pickerFireColor{(float) fireColor.r, (float) fireColor.g, (float) fireColor.b,
                                          (float) fireColor.a};

            ImGui::SetNextWindowSize(ImVec2(0, 0));
            ImGui::Begin("Colors", &colorWindow);
            ImGui::SetNextItemWidth(100);

            if (ImGui::ColorPicker4("Tree", (float *) &pickerTreeColor,
                                    ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs,
                                    (float *) &RESET_TREE_COLOR))
                treeColor = {(unsigned char) (pickerTreeColor.x * 255.0), (unsigned char) (pickerTreeColor.y * 255.0),
                             (unsigned char) (pickerTreeColor.z * 255.0), DEFAULT_TREE_COLOR.a};

            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);

            if (ImGui::ColorPicker4("Fire", (float *) &pickerFireColor,
                                    ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs,
                                    (float *) &RESET_FIRE_COLOR))
                fireColor = {(unsigned char) (pickerFireColor.x * 255.0), (unsigned char) (pickerFireColor.y * 255.0),
                             (unsigned char) (pickerFireColor.z * 255.0), DEFAULT_FIRE_COLOR.a};


            if (ImGui::Button("Reset colors")) {
                treeColor = DEFAULT_TREE_COLOR;
                fireColor = DEFAULT_FIRE_COLOR;

                pickerTreeColor = {(float) treeColor.r, (float) treeColor.g, (float) treeColor.b,
                                   (float) treeColor.a};
                pickerFireColor = {(float) fireColor.r, (float) fireColor.g, (float) fireColor.b,
                                   (float) fireColor.a};

            }

            ImGui::End();
        }

        if (!limitAnimation)
            calculateForest = true;

        if (stepwiseAnimation && !animationStep)
            calculateForest = false;

        if (calculateForest) {
            stepForest(fire, growth);
            calculateForest = false;
        }

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8) (clearColor.x * 255), (Uint8) (clearColor.y * 255),
                               (Uint8) (clearColor.z * 255), (Uint8) (clearColor.w * 255));
        SDL_RenderClear(renderer);

        for (int i = 0; i < currentWidth; ++i)
            for (int j = 0; j < currentHeight; ++j)
                if (forest[i][j] == TREE)
                    drawSquare(i, j, treeColor); // Green for tree
                else if (forest[i][j] == FIRE)
                    drawSquare(i, j, fireColor); // Red for fire

        // End frame timing
        unsigned int endTicks = SDL_GetTicks();
        unsigned long long endPerf = SDL_GetPerformanceCounter();

        measurements.framePerf = endPerf - startPerf;
        measurements.fps = 1 / (((float) endTicks - (float) startTicks) / 1000.0f);
        totalFrameTicks += endTicks - startTicks;
        measurements.avg = 1000.0f / ((float) totalFrameTicks / (float) totalFrames);

        if (limitAnimation) {
            float dT = ((float) endTicks - (float) lastUpdate) / 1000.0f;
            int framesToUpdate = floor(dT / (1.0f / currentSpeed));
            if (framesToUpdate > 0) {
                lastFrame += framesToUpdate;
                lastUpdate = endTicks;
                calculateForest = true;
            }
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);

        if (startMeasure) {
            currentStep++;
            progressCurrentStep++;
        }

        if (animationStep)
            animationStep = false;
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