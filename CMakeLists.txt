cmake_minimum_required(VERSION 3.14)
project(fm_synth_gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# RtAudio
FetchContent_Declare(
        rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG master
)
FetchContent_MakeAvailable(rtaudio)

# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Add imgui backend sources manually
set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(fm_synth_gui
        main.cpp
        ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(fm_synth_gui PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${rtaudio_SOURCE_DIR}
        ${glfw_SOURCE_DIR}/include
)

# Link libraries
find_package(OpenGL REQUIRED)
target_link_libraries(fm_synth_gui PRIVATE
        rtaudio
        glfw
        OpenGL::GL
)

# macOS frameworks
if(APPLE)
    target_link_libraries(fm_synth_gui PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
            "-framework CoreAudio"
            "-framework AudioUnit"
    )
endif()
