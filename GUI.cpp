void createWindow() {
    auto windowFlags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("Forest fire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              currentWidth * currentSize,
                              currentHeight * currentSize, windowFlags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
}

void mainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Reset")) {
                resetSimulation();
            }
            if (ImGui::MenuItem("Exit", "Cmd+Q")) {
                stopSimulation();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings", nullptr, &settingsWindow);
            ImGui::MenuItem("Measurements", nullptr, &measurementWindow);
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
        ImGui::SliderFloat("Spontaneous fire", &fire, 0.0f, 0.01f, "%.4f");
        ImGui::SliderFloat("Tree growth", &growth, 0.0f, 0.1f, "%.3f");
        const char *items[] = {"Von Neumann", "Moore"};
        static const char *current_item = items[0];

        if (lastHeight != currentHeight || lastWidth != currentWidth || lastSize != currentSize) {
            SDL_SetWindowSize(window, currentWidth * currentSize, currentHeight * currentSize);
            lastHeight = currentHeight;
            lastWidth = currentWidth;
            lastSize = currentSize;
        }

        if (ImGui::BeginCombo("Neighborhood logic", current_item)) {
            for (auto &item: items) {
                bool is_selected = (current_item == item);
                if (ImGui::Selectable(item, is_selected))
                    current_item = item;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                if ((items[0] == current_item) && (currentLogic == MOORE))
                    currentLogic = VON_NEUMANN;
                else if ((items[1] == current_item) && (currentLogic == VON_NEUMANN))
                    currentLogic = MOORE;
            }
            ImGui::EndCombo();
        }
        ImGui::End();
    }
}