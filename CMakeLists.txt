cmake_minimum_required(VERSION 3.14)
project(fm_drum_synth)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# RtAudio
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build as shared library" FORCE)
FetchContent_Declare(
        rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG 5.2.0
)
FetchContent_MakeAvailable(rtaudio)

# GLFW
set(GLFW_BUILD_SHARED_LIBS OFF CACHE BOOL "Build static glfw" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

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

# ImGui backend sources
set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Your source files
file(GLOB MODEL_SOURCES
        "Fm*.cpp"
        "TRXBassDrum.cpp"
        "TRXSnareDrum.cpp"
        "TRXClaves.cpp"
        "TRXHiHat.cpp"
)

add_executable(fm_drum_synth
        main.cpp
        CustomControls.cpp
        glad.c
        ${MODEL_SOURCES}
        ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(fm_drum_synth PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${rtaudio_SOURCE_DIR}
        ${glfw_SOURCE_DIR}/include
        .
)

# Link libraries
find_package(OpenGL REQUIRED)

# Link statically to rtaudio and glfw
# Ensure static linkage
if(APPLE)
  target_link_libraries(fm_drum_synth PRIVATE
    rtaudio
    glfw
    OpenGL::GL
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework CoreAudio"
    "-framework AudioUnit"
  )
else()
  target_link_libraries(fm_drum_synth PRIVATE rtaudio glfw OpenGL::GL)
endif()

# Use GLSL 130 for macOS OpenGL compatibility
#add_definitions(-DGLSL_VERSION_STR="\\\"#version 130\\\"")
