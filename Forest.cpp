#include <iostream>
#include <random>
#include <queue>
#include <vector>

#include <omp.h>

const int SIZE = 1;
const int WIDTH = 1024;
const int HEIGHT = 1024;

const double START_GROWTH = 0.5;
const float BASE_FIRE = 0.0001;
const float BASE_GROWTH = 0.02;

enum CellState {
    TREE, FIRE, EMPTY
};

enum NeighborhoodLogic {
    MOORE,
    VON_NEUMANN
};

NeighborhoodLogic currentLogic{VON_NEUMANN};

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

bool running{true};
bool startMeasure{false};

bool settingsWindow{false};
bool measurementWindow{false};

int currentStep{0};
int maxSteps{0};

float progressAllSteps(11111.0);
float progressCurrentStep{0.0};

float fire{BASE_FIRE};
float growth{BASE_GROWTH};

int currentSize{SIZE};
int currentWidth{WIDTH};
int currentHeight{HEIGHT};

ImVec4 clearColor{ImVec4(0.94f, 0.94f, 0.94f, 1.00f)};

std::vector<std::vector<CellState>> forest(currentWidth, std::vector<CellState>(currentHeight, EMPTY));

int num_steps[] = {1, 10, 100, 1000, 10000}; // array to hold different step values
std::priority_queue<int, std::vector<int>, std::greater<>> measureSteps;

void initForest() {
    std::mt19937 rng_init(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

#pragma omp parallel for collapse(2) default(none) shared(forest, START_GROWTH, currentWidth, currentHeight) private(dist, rng_init)
    for (int i = 0; i < currentWidth; ++i) {
        for (int j = 0; j < currentHeight; ++j) {
            forest[i][j] = (dist(rng_init) < START_GROWTH) ? TREE : EMPTY;
        }
    }
}


void drawSquare(int x, int y, SDL_Color color) {
    SDL_Rect fillRect = {x * currentSize, y * currentSize, currentSize, currentSize};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &fillRect);
}

bool isFireNearby(int x, int y, NeighborhoodLogic logic) {
    // Von Neumann neighborhood: up, down, left, right
    if ((x > 0 && forest[x - 1][y] == FIRE) ||
        (y > 0 && forest[x][y - 1] == FIRE) ||
        (x < currentWidth - 1 && forest[x + 1][y] == FIRE) ||
        (y < currentHeight - 1 && forest[x][y + 1] == FIRE)) {
        return true;
    }

    // If Moore neighborhood is not selected, or we found fire in Von Neumann neighborhood, no need to check further.
    if (logic == VON_NEUMANN) {
        return false;
    }

    // Moore neighborhood: also consider diagonals
    return (x > 0 && y > 0 && forest[x - 1][y - 1] == FIRE) ||
           (x < currentWidth - 1 && y > 0 && forest[x + 1][y - 1] == FIRE) ||
           (x > 0 && y < currentHeight - 1 && forest[x - 1][y + 1] == FIRE) ||
           (x < currentWidth - 1 && y < currentHeight - 1 && forest[x + 1][y + 1] == FIRE);
}

void stepForest(double p, double g) {
    std::vector<std::vector<CellState>> newForest = forest;

    // Prepare RNGs for each thread
    int max_threads = omp_get_max_threads();
    std::vector<std::mt19937> randomGens(max_threads);
    for (int i = 0; i < max_threads; ++i) {
        randomGens[i].seed(std::chrono::high_resolution_clock::now().time_since_epoch().count() + i);
    }
    std::uniform_real_distribution<double> dist(0.0, 1.0);

#pragma omp parallel for collapse(2) default(none) shared(forest, newForest, currentLogic, p, g, randomGens, currentWidth, currentHeight) private(dist)
    for (int x = 0; x < currentWidth; ++x) {
        for (int y = 0; y < currentHeight; ++y) {
            std::mt19937 &rng_local = randomGens[omp_get_thread_num()];

            if (forest[x][y] == FIRE) {
                newForest[x][y] = EMPTY;
            } else if (forest[x][y] == TREE) {
                bool fireNearby = isFireNearby(x, y, currentLogic);

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
    currentSize = SIZE;
    currentWidth = WIDTH;
    currentHeight = HEIGHT;
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
    maxSteps = measureSteps.top();
    measureSteps.pop();
}