cmake_minimum_required(VERSION 3.14)
project(fm_drum_synth)

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
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(RTAUDIO_STATIC ON CACHE BOOL "Build static rtaudio" FORCE)
set(GLFW_BUILD_SHARED_LIBS OFF CACHE BOOL "Build static glfw" FORCE)

# Ensure static linkage
set_target_properties(rtaudio PROPERTIES IMPORTED_LOCATION_RELEASE "${rtaudio_BINARY_DIR}/librtaudio.a")
set_target_properties(glfw PROPERTIES IMPORTED_LOCATION_RELEASE "${glfw_BINARY_DIR}/src/libglfw3.a")

# Link statically
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
