#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <memory>
#include <RtAudio.h>
#include <fstream>
#include <filesystem>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "DrumModel.h"
#include "FmKickModel.h"
#include "FmSnareModel.h"
#include "FmTomModel.h"
#include "FmClapModel.h"
#include "FmRimshotModel.h"
#include "FmCowbellModel.h"
#include "FmCymbalModel.h"
#include "HiHatClosedModel.h"

#include "CustomControls.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "resources/background_png.h"

constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float SAMPLE_RATE = 48000.0f;
constexpr size_t BUFFER_SIZE = 256;

std::mutex param_mutex;
std::atomic<bool> trigger_requested(false);
std::atomic<size_t> selected_model_index = 0;

std::vector<std::shared_ptr<DrumModel>> models;
std::vector<std::string> model_names;

GLuint gBackgroundTex = 0;
int gBackgroundW = 0, gBackgroundH = 0;

void LoadBackgroundTexture() {
    int n;
    // Use the correct symbol names from background_png.h
    unsigned char* data = stbi_load_from_memory(resources_background_png, resources_background_png_len, &gBackgroundW, &gBackgroundH, &n, 4);
    if (!data) return;
    glGenTextures(1, &gBackgroundTex);
    glBindTexture(GL_TEXTURE_2D, gBackgroundTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gBackgroundW, gBackgroundH, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

void RenderBackground() {
    if (!gBackgroundTex) return;
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    draw_list->AddImage((void*)(intptr_t)gBackgroundTex, ImVec2(0,0), ImVec2((float)io.DisplaySize.x, (float)io.DisplaySize.y), ImVec2(0,0), ImVec2(1,1));
}

int audioCallback(void* outputBuffer, void*, unsigned int nBufferFrames, double, RtAudioStreamStatus, void*) {
    float* out = reinterpret_cast<float*>(outputBuffer);
    for (unsigned int i = 0; i < nBufferFrames; ++i) {
        if (trigger_requested.exchange(false)) {
            std::lock_guard<std::mutex> lock(param_mutex);
            models[selected_model_index]->Trigger();
        }
        float sample = models[selected_model_index]->Process();
        out[2 * i] = sample;     // Left channel
        out[2 * i + 1] = sample; // Right channel
    }
    return 0;
}

void ShowControls() {
    ImGui::Begin("FM Drum Synth");

    if (ImGui::BeginCombo("Drum Model", model_names[selected_model_index].c_str())) {
        for (size_t i = 0; i < model_names.size(); ++i) {
            bool selected = (selected_model_index == i);
            if (ImGui::Selectable(model_names[i].c_str(), selected)) {
                selected_model_index = i;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Trigger (t or space)")) {
        trigger_requested = true;
    }

    CustomControls::BeginParameters();

    std::lock_guard<std::mutex> lock(param_mutex);
    models[selected_model_index]->RenderControls();

    CustomControls::EndParameters();

    ImGui::End();
}

void ShowMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Parameters\tCtrl+S")) {
                std::ofstream ofs("drum_params.txt");
                if (ofs) {
                    std::lock_guard<std::mutex> lock(param_mutex);
                    for (const auto& model : models) {
                        model->saveParameters(ofs);
                    }
                }
            }
            if (ImGui::MenuItem("Load Parameters\tCtrl+L")) {
                std::ifstream ifs("drum_params.txt");
                if (ifs) {
                    std::lock_guard<std::mutex> lock(param_mutex);
                    for (const auto& model : models) {
                        model->loadParameters(ifs);
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

int main() {
    models.push_back(std::make_shared<FmKickModel>()); model_names.push_back("Kick");
    models.push_back(std::make_shared<FmSnareModel>()); model_names.push_back("Snare");
    models.push_back(std::make_shared<FmTomModel>()); model_names.push_back("Tom");
    models.push_back(std::make_shared<FmClapModel>()); model_names.push_back("Clap");
    models.push_back(std::make_shared<FmRimshotModel>()); model_names.push_back("Rimshot");
    models.push_back(std::make_shared<FmCowbellModel>()); model_names.push_back("Cowbell");
    models.push_back(std::make_shared<FmCymbalModel>()); model_names.push_back("Cymbal");
    models.push_back(std::make_shared<HiHatClosedModel>()); model_names.push_back("HiHat Closed");

    for (auto& model : models) model->Init();

    // Load last parameters at program start, or create with defaults if missing
    namespace fs = std::filesystem;
    const char* param_file = "drum_params.txt";
    if (!fs::exists(param_file)) {
        std::ofstream ofs(param_file);
        ofs << "59.016 0.091 116.189 0.02 0.23 0.012 149.18 0.04\n";
        ofs << "160.247 0.091 551.229 1.025 0.01 0.5 0.124 799.016\n";
        ofs << "146.885 0.024 100.04 0.04 1 17.213 0.027\n";
        ofs << "176.64 1585.66 15.164 0.095 0.01 0.09 3 0.034 953.197 0.018\n";
        ofs << "288.525 0.01 1.23 198.033 0.05 3.074 0.119 0.018 427.049\n";
        ofs << "281.967 0.03 0.099 879.098 0.02 0.042 0.008 0.279\n";
        ofs << "295.492 745.902 0.066 15.123 0.234 0.049 0 1197.95\n";
    }
    {
        std::ifstream ifs(param_file);
        if (ifs) {
            std::lock_guard<std::mutex> lock(param_mutex);
            for (const auto& model : models) {
                model->loadParameters(ifs);
            }
        }
    }

    RtAudio dac;
    if (dac.getDeviceCount() < 1) {
        std::cerr << "No audio devices found!\n";
        return 1;
    }

    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 2; // Set to stereo
    parameters.firstChannel = 0;

    unsigned int bufferFrames = BUFFER_SIZE;
    dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32, SAMPLE_RATE, &bufferFrames, &audioCallback, nullptr);
    dac.startStream();

    glfwInit();
    // Use OpenGL 3.2+ core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS

    GLFWwindow* window = glfwCreateWindow(640, 480, "FM Synth", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    LoadBackgroundTexture();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        RenderBackground();

        // Hotkey: Ctrl+S to save, Ctrl+L to load
        ImGuiIO& io = ImGui::GetIO();
        bool ctrl = io.KeyCtrl;
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            std::ofstream ofs("drum_params.txt");
            if (ofs) {
                std::lock_guard<std::mutex> lock(param_mutex);
                for (const auto& model : models) {
                    model->saveParameters(ofs);
                }
            }
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_L, false)) {
            std::ifstream ifs("drum_params.txt");
            if (ifs) {
                std::lock_guard<std::mutex> lock(param_mutex);
                for (const auto& model : models) {
                    model->loadParameters(ifs);
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            trigger_requested = true;
        }

        ShowMenuBar();
        ShowControls();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}