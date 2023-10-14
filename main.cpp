#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <vector>
#include <random>
#include <omp.h>

const int SIZE = 5;
const int WIDTH = 1024;
const int HEIGHT = 1024;

const double INIT_GROWTH  = 0.5;
const float BASE_FIRE = 0.5;
const float BASE_GROWTH  = 0.5;

enum CellState {
    TREE, FIRE, EMPTY
};


SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

std::vector<std::vector<CellState>> forest(WIDTH, std::vector<CellState>(HEIGHT, EMPTY));

bool running{true};

float fire{BASE_FIRE};
float growth{BASE_GROWTH};

void initForest(double init_growth) {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 rng_init(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (int i = 0; i < WIDTH; ++i) {
        for (int j = 0; j < HEIGHT; ++j) {
            forest[i][j] = (dist(rng_init) < init_growth) ? TREE : EMPTY;
        }
    }
}


void drawSquare(int x, int y, SDL_Color color) {
    SDL_Rect fillRect = {x * SIZE, y * SIZE, SIZE, SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &fillRect);
}

void stepForest(double p, double g) {
    std::vector<std::vector<CellState>> newForest = forest;

    // Prepare RNGs for each thread
    int max_threads = omp_get_max_threads();
    std::vector<std::mt19937> rngs(max_threads);
    for (int i = 0; i < max_threads; ++i) {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count() + i;
        rngs[i].seed(seed);
    }
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    #pragma omp parallel for collapse(2) default(none) shared(forest, newForest, p, g, rngs, WIDTH, HEIGHT) private(dist)
    for (int x = 0; x < WIDTH; ++x) {
        for (int y = 0; y < HEIGHT; ++y) {
            // Use the RNG for the current thread
            std::mt19937 &rng_local = rngs[omp_get_thread_num()];

            if (forest[x][y] == FIRE) {
                newForest[x][y] = EMPTY;
            } else if (forest[x][y] == TREE) {
                bool fireNearby =
                        (x > 0 && forest[x - 1][y] == FIRE) ||
                        (y > 0 && forest[x][y - 1] == FIRE) ||
                        (x < WIDTH - 1 && forest[x + 1][y] == FIRE) ||
                        (y < HEIGHT - 1 && forest[x][y + 1] == FIRE);

                if (fireNearby || dist(rng_local) < p) {
                    newForest[x][y] = FIRE;
                }
            } else if (forest[x][y] == EMPTY) {
                if (dist(rng_local) < g) {
                    newForest[x][y] = TREE;
                }
            }
        }
    }

    forest = newForest;
}

void stop() {
    running = false;
}

void reset() {
    fire = BASE_FIRE;
    growth = BASE_GROWTH;
    initForest(INIT_GROWTH);
}

// Main code
int main(int, char **) {
    bool probability_window = true;

    // ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    // ImVec4 clear_color = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
    ImVec4 clear_color = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);


    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Create window with SDL_Renderer graphics context
    auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("Forest fire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * SIZE,
                              HEIGHT * SIZE, window_flags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        SDL_Log("Error creating SDL_Renderer!");
        return EXIT_FAILURE;
    }

    initForest(INIT_GROWTH);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Main loop
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                stop();
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                stop();
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ImGui::DockSpaceOverViewport();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Reset", "Cmd+R")) {
                    stop();
                }
                if (ImGui::MenuItem("Exit", "Cmd+Q")) {
                    stop();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Probabilities", nullptr, &probability_window);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (probability_window) {
            ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f));
            ImGui::Begin("Probabilities", &probability_window);
            ImGui::SliderFloat("Spontaneous fire", &fire, 0.0f, 1.0f);
            ImGui::SliderFloat("Tree growth", &growth, 0.0f, 1.0f);
            ImGui::End();
        }

        stepForest(fire, growth);

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8) (clear_color.x * 255), (Uint8) (clear_color.y * 255),
                               (Uint8) (clear_color.z * 255), (Uint8) (clear_color.w * 255));
        SDL_RenderClear(renderer);

        for (int i = 0; i < WIDTH; ++i) {
            for (int j = 0; j < HEIGHT; ++j) {
                if (forest[i][j] == TREE) {
                    drawSquare(i, j, {0, 128, 0, 255}); // Green for tree
                } else if (forest[i][j] == FIRE) {
                    drawSquare(i, j, {255, 0, 0, 255}); // Red for fire
                }
            }
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
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