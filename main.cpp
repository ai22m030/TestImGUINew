#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <queue>
#include <vector>
#include <random>
#include <omp.h>

const int SIZE = 5;
const int WIDTH = 1024;
const int HEIGHT = 1024;

const double START_GROWTH = 0.5;
const float BASE_FIRE = 0.0001;
const float BASE_GROWTH = 0.02;

enum CellState {
    TREE, FIRE, EMPTY
};

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

bool running{true};
bool startMeasure{false};

bool probabilityWindow{false};
bool measurementWindow{false};

int currentStep{0};
int maxSteps{0};

float progressAllSteps(11111.0);
float progressCurrentStep{0.0};

float fire{BASE_FIRE};
float growth{BASE_GROWTH};

ImVec4 clearColor{ImVec4(0.94f, 0.94f, 0.94f, 1.00f)};

std::vector<std::vector<CellState>> forest(WIDTH, std::vector<CellState>(HEIGHT, EMPTY));

int num_steps[] = {1, 10, 100, 1000, 10000}; // array to hold different step values
std::priority_queue<int, std::vector<int>, std::greater<>> min_heap;

struct MeasurementsLog {
    ImGuiTextBuffer Buf;
    ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.

    MeasurementsLog() {
        Clear();
    }

    void Clear() {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void AddLog(const char *fmt, ...) IM_FMTARGS(2) {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void Draw(const char *title, bool *p_open = nullptr) {
        if (!ImGui::Begin(title, p_open)) {
            ImGui::End();
            return;
        }

        // Main window
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");

        ImGui::Separator();

        if (ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            if (clear)
                Clear();
            if (copy)
                ImGui::LogToClipboard();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            const char *buf = Buf.begin();
            const char *buf_end = Buf.end();
            ImGuiListClipper clipper;
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step()) {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
                    const char *line_start = buf + LineOffsets[line_no];
                    const char *line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1)
                                                                            : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();

            ImGui::PopStyleVar();

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
    }
};

void initForest() {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 rng_init(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

#pragma omp parallel for collapse(2) default(none) shared(forest, START_GROWTH, WIDTH, HEIGHT) private(dist, rng_init)
    for (int i = 0; i < WIDTH; ++i) {
        for (int j = 0; j < HEIGHT; ++j) {
            forest[i][j] = (dist(rng_init) < START_GROWTH) ? TREE : EMPTY;
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
    std::vector<std::mt19937> randomGens(max_threads);
    for (int i = 0; i < max_threads; ++i) {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count() + i;
        randomGens[i].seed(seed);
    }
    std::uniform_real_distribution<double> dist(0.0, 1.0);

#pragma omp parallel for collapse(2) default(none) shared(forest, newForest, p, g, randomGens, WIDTH, HEIGHT) private(dist)
    for (int x = 0; x < WIDTH; ++x) {
        for (int y = 0; y < HEIGHT; ++y) {
            // Use the RNG for the current thread
            std::mt19937 &rng_local = randomGens[omp_get_thread_num()];

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

void stopSimulation() {
    running = false;
}

void resetSimulation() {
    fire = BASE_FIRE;
    growth = BASE_GROWTH;
    initForest();
}

void resetMeasure() {
    startMeasure = false;
    currentStep = 0;
    progressCurrentStep = 0;
    maxSteps = 0;
}

void nextSteps() {
    currentStep = 0;
    maxSteps = min_heap.top();
    min_heap.pop();
}

// Main code
int main(int, char **) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Create window with SDL_Renderer graphics context
    auto windowFlags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("Forest fire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * SIZE,
                              HEIGHT * SIZE, windowFlags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        SDL_Log("Error creating SDL_Renderer!");
        return EXIT_FAILURE;
    }

    auto start = std::chrono::high_resolution_clock::now();
    static MeasurementsLog measurements;

    initForest();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

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

                    int gridX = x / SIZE;
                    int gridY = y / SIZE;

                    // Start a fire at the clicked location
                    if (forest[gridX][gridY] == TREE) {
                        forest[gridX][gridY] = FIRE;
                    }
                }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

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
                ImGui::MenuItem("Probabilities", nullptr, &probabilityWindow);
                ImGui::MenuItem("Measurements", nullptr, &measurementWindow);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (probabilityWindow) {
            ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f));
            ImGui::Begin("Probabilities", &probabilityWindow);
            ImGui::SliderFloat("Spontaneous fire", &fire, 0.0f, 0.01f, "%.4f");
            ImGui::SliderFloat("Tree growth", &growth, 0.0f, 0.1f, "%.3f");
            ImGui::End();
        }

        if ((currentStep == maxSteps) && startMeasure) {
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

            measurements.AddLog("[%s] Time taken for %i steps: %lld ms\n", "info", maxSteps, duration.count());

            if (!min_heap.empty()) {
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
                        min_heap = std::priority_queue<int, std::vector<int>, std::greater<>>(num_steps, num_steps + 5);
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

        for (int i = 0; i < WIDTH; ++i) {
            for (int j = 0; j < HEIGHT; ++j) {
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