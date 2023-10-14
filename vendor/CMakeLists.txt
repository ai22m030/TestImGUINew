include(FetchContent)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG ff534b0d0a6df31764626a97ebcaa0de5be20fd3 # Branch: docking, date: 02.10.2023, 10:57 GMT+2
)
add_subdirectory(imgui)
