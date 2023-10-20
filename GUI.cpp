#include <SDL_render.h>
#include <SDL_video.h>
#include <imgui.h>

void createWindow() {
    auto windowFlags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("ForestFire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              currentWidth * currentSize,
                              currentHeight * currentSize, windowFlags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
}

void mainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Reset"))
                initForest();

            if (ImGui::MenuItem("Exit", "Cmd+Q"))
                running = false;

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings", nullptr, &settingsWindow);
            ImGui::MenuItem("Measurements", nullptr, &measurementWindow);
            ImGui::MenuItem("Colors", nullptr, &colorWindow);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void initSettings(int &lastHeight, int &lastWidth, int &lastSize) {
    if (settingsWindow) {
        ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f));

        ImGui::Begin("Settings", &settingsWindow);

        ImGui::SliderInt("Height", &currentHeight, 100, 1024);
        ImGui::SliderInt("Width", &currentWidth, 100, 1024);
        ImGui::SliderInt("Zoom", &currentSize, 1, 5);

        ImGui::SliderFloat("Spontaneous fire", &fire, 0.0f, 0.005f, "%.4f");
        ImGui::SliderFloat("Tree growth", &growth, 0.0f, 0.3f, "%.3f");

        const char *items[] = {"Von Neumann", "Moore"};
        static const char *currentItem = items[0];

        if (lastHeight != currentHeight || lastWidth != currentWidth || lastSize != currentSize) {
            SDL_SetWindowSize(window, currentWidth * currentSize, currentHeight * currentSize);
            lastHeight = currentHeight;
            lastWidth = currentWidth;
            lastSize = currentSize;
        }

        if (ImGui::BeginCombo("Neighborhood logic", currentItem)) {
            for (auto &item: items) {
                bool isSelected = (currentItem == item);
                if (ImGui::Selectable(item, isSelected))
                    currentItem = item;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();

                if ((items[0] == currentItem) && (currentLogic == MOORE))
                    currentLogic = VON_NEUMANN;
                else if ((items[1] == currentItem) && (currentLogic == VON_NEUMANN))
                    currentLogic = MOORE;
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        if (limitAnimation && !stepwiseAnimation) {
            ImGui::Checkbox("Limit ", &limitAnimation);
            ImGui::SameLine();
            ImGui::SliderFloat(" FPS", &currentSpeed, 1.0f, 50.0f, "%.f");
        } else if (!stepwiseAnimation) {
            ImGui::Checkbox(" Speed control", &limitAnimation);
        }

        if (!stepwiseAnimation)
            ImGui::Separator();

        if (stepwiseAnimation && !limitAnimation) {
            ImGui::Checkbox(" Step animation", &stepwiseAnimation);
            ImGui::SameLine();

            if (!animationStep)
                if (ImGui::Button("Make step"))
                    animationStep = true;
        } else if (!limitAnimation) {
            ImGui::Checkbox(" Step animation", &stepwiseAnimation);
        }

        ImGui::End();
    }
}