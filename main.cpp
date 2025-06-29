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
#include <deque>
#include <complex>
#include <algorithm>

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
#include "TRXBassDrum.h"
#include "TRXSnareDrum.h"
#include "TRXClaves.h"
#include "TRXHiHat.h"

#include "CustomControls.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "resources/background_png.h"

constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float SAMPLE_RATE = 48000.0f;
constexpr size_t BUFFER_SIZE = 256;
constexpr size_t WAVEFORM_BUFFER_SIZE = 2048;
constexpr size_t FFT_SIZE = 256;
constexpr size_t WATERFALL_HISTORY = 256;

std::mutex param_mutex;
std::atomic<bool> trigger_requested(false);
std::atomic<size_t> selected_model_index = 0;

std::vector<std::shared_ptr<DrumModel>> models;
std::vector<std::string> model_names;

GLuint gBackgroundTex = 0;
int gBackgroundW = 0, gBackgroundH = 0;

std::deque<float> waveformBuffer;
std::mutex waveformMutex;

std::vector<std::vector<float>> waterfallHistory(WATERFALL_HISTORY, std::vector<float>(FFT_SIZE/2, 0.0f));
size_t waterfallPos = 0;
std::vector<float> fftInputBuffer;
std::mutex fftMutex;

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

// Minimal in-place Radix-2 FFT (real input, magnitude output)
void computeFFT(const std::vector<float>& in, std::vector<float>& out) {
    size_t N = in.size();
    std::vector<std::complex<float>> data(N);
    for (size_t i = 0; i < N; ++i) data[i] = in[i];
    // Bit reversal
    size_t j = 0;
    for (size_t i = 0; i < N; ++i) {
        if (i < j) std::swap(data[i], data[j]);
        size_t m = N >> 1;
        while (m && j >= m) { j -= m; m >>= 1; }
        j += m;
    }
    // FFT
    for (size_t s = 1; s <= (size_t)log2(N); ++s) {
        size_t m = 1 << s;
        std::complex<float> wm = std::exp(std::complex<float>(0, -2.0f * PI / m));
        for (size_t k = 0; k < N; k += m) {
            std::complex<float> w = 1;
            for (size_t l = 0; l < m/2; ++l) {
                auto t = w * data[k + l + m/2];
                auto u = data[k + l];
                data[k + l] = u + t;
                data[k + l + m/2] = u - t;
                w *= wm;
            }
        }
    }
    // Output magnitude (first N/2 bins)
    out.resize(N/2);
    for (size_t i = 0; i < N/2; ++i) {
        out[i] = std::abs(data[i]) / (float)N;
    }
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
        // Store sample for waveform display
        {
            std::lock_guard<std::mutex> lock(waveformMutex);
            waveformBuffer.push_back(sample);
            if (waveformBuffer.size() > WAVEFORM_BUFFER_SIZE) {
                waveformBuffer.pop_front();
            }
        }
        // Collect samples for FFT
        {
            std::lock_guard<std::mutex> lock(fftMutex);
            fftInputBuffer.push_back(sample);
            if (fftInputBuffer.size() > FFT_SIZE) {
                fftInputBuffer.erase(fftInputBuffer.begin(), fftInputBuffer.begin() + (fftInputBuffer.size() - FFT_SIZE));
            }
        }
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

    if (ImGui::Button("Trigger (space)")) {
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

void ShowWaveformWindow() {
    ImGui::Begin("Waveform Display", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    std::vector<float> samples;
    {
        std::lock_guard<std::mutex> lock(waveformMutex);
        samples.assign(waveformBuffer.begin(), waveformBuffer.end());
    }
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (!samples.empty()) {
        ImGui::PlotLines("Waveform", samples.data(), (int)samples.size(), 0, nullptr, -1.0f, 1.0f, avail);
    } else {
        ImGui::Text("No waveform data.");
    }
    ImGui::End();
}

void ShowWaterfallWindow() {
    static std::vector<unsigned char> image(WATERFALL_HISTORY * (FFT_SIZE/2) * 3, 0);
    static GLuint waterfallTex = 0;
    // Compute FFT and update waterfall
    std::vector<float> fftBuf;
    {
        std::lock_guard<std::mutex> lock(fftMutex);
        if (fftInputBuffer.size() == FFT_SIZE) {
            computeFFT(fftInputBuffer, waterfallHistory[waterfallPos]);
            waterfallPos = (waterfallPos + 1) % WATERFALL_HISTORY;
        }
    }
    // Fill image buffer with rotated (horizontal scroll, 180 deg flip) spectrogram: time=X, freq=Y
    for (size_t x = 0; x < WATERFALL_HISTORY; ++x) {
        size_t col = (waterfallPos + WATERFALL_HISTORY - 1 - x) % WATERFALL_HISTORY;
        for (size_t y = 0; y < FFT_SIZE/2; ++y) {
            size_t fy = (FFT_SIZE/2 - 1) - y;
            float v = std::min(1.0f, waterfallHistory[col][y] * 20.0f);
            unsigned char c = (unsigned char)(v * 255);
            size_t idx = 3 * (fy * WATERFALL_HISTORY + x);
            image[idx + 0] = c;
            image[idx + 1] = c;
            image[idx + 2] = c;
        }
    }
    // Create or update OpenGL texture (width=WATERFALL_HISTORY, height=FFT_SIZE/2)
    if (!waterfallTex) {
        glGenTextures(1, &waterfallTex);
        glBindTexture(GL_TEXTURE_2D, waterfallTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WATERFALL_HISTORY, FFT_SIZE/2, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    } else {
        glBindTexture(GL_TEXTURE_2D, waterfallTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WATERFALL_HISTORY, FFT_SIZE/2, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    }
    ImGui::Begin("Waterfall (Spectrogram)", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    // Fill the window completely, even if aspect ratio is not preserved
    ImGui::Image((void*)(intptr_t)waterfallTex, avail);
    ImGui::End();
}

int main() {
    models.push_back(std::make_shared<FmKickModel>()); model_names.push_back("Kick");
    models.push_back(std::make_shared<FmSnareModel>()); model_names.push_back("Snare");
    models.push_back(std::make_shared<FmTomModel>()); model_names.push_back("Tom");
    models.push_back(std::make_shared<FmClapModel>()); model_names.push_back("Clap");
    models.push_back(std::make_shared<FmRimshotModel>()); model_names.push_back("Rimshot");
    models.push_back(std::make_shared<FmCowbellModel>()); model_names.push_back("Cowbell");
    models.push_back(std::make_shared<FmCymbalModel>()); model_names.push_back("Cymbal");
    models.push_back(std::make_shared<TRXBassDrum>()); model_names.push_back("TRX Bass Drum");
    models.push_back(std::make_shared<TRXSnareDrum>()); model_names.push_back("TRX Snare Drum");
    models.push_back(std::make_shared<TRXClaves>()); model_names.push_back("TRX Claves");
    models.push_back(std::make_shared<TRXHiHat>()); model_names.push_back("TRX HiHat");

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
        ofs << "322.476 0.015 3.42 120.651 0.05 2.911 0.5 0.02 742.684\n";
        ofs << "281.967 0.03 0.099 879.098 0.02 0.042 0.008 0.279\n";
        ofs << "295.492 745.902 0.066 15.123 0.234 0.049 0 1197.95\n";
        ofs << "30.098 0.088 0.075 0.033 1.785 0.088 0.583 0.368\n";
        ofs << "176.287 0.065 0.733 0.169 0.388 105.537 0.1 0.2\n";
        ofs << "880.782 280.13 0.01 0.912 0\n";
        ofs << "0.489 0.084 8990.23 5678.83 0.759 0.583\n";
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

        ShowMenuBar();
        ShowControls();
        ShowWaveformWindow();
        ShowWaterfallWindow();

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    dac.stopStream();
    dac.closeStream();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
