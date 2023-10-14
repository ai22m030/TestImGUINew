cmake_minimum_required(VERSION 3.23)
project(TestImGUINew)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fopenmp")

add_executable(${PROJECT_NAME} main.cpp)

INCLUDE(FindPkgConfig)

find_package(OpenMP QUIET)

if (OpenMP_CXX_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX_INCLUDE_DIRS)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif ()

set(SDL2_LIB "/Library/Frameworks/SDL2.framework/SDL2")
set(SDL2_HEAD "/Library/Frameworks/SDL2.framework/Headers")
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_HEAD})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIB} imgui)

add_subdirectory(vendor)