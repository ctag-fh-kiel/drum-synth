#include "glad.h"
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
constexpr size_t WAVEFORM_BUFFER_SIZE = 48000;
constexpr size_t FFT_SIZE = 256;
constexpr size_t WATERFALL_HISTORY = 256;

std::mutex param_mutex;
std::atomic<bool> trigger_requested(false);
std::atomic<size_t> selected_model_index = 0;

std::vector<std::shared_ptr<DrumModel>> models;
std::vector<std::string> model_names;

GLuint gBackgroundTex = 0;
int gBackgroundW = 0, gBackgroundH = 0;

// Add a handle for the FFT texture
GLuint gFftTex = 0;
GLint bgFftTexLoc = -1;

std::deque<float> waveformBuffer;
std::mutex waveformMutex;

std::vector<std::vector<float>> waterfallHistory(WATERFALL_HISTORY, std::vector<float>(FFT_SIZE/2, 0.0f));
size_t waterfallPos = 0;
std::vector<float> fftInputBuffer;
std::mutex fftMutex;

// GLSL animated background shader (simple color pulse based on loudness)
const char* bgVertexShaderSrc = R"(
#version 150 core
in vec2 pos;
out vec2 uv;
void main() {
    uv = (pos + 1.0) * 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

// Update the fragment shader for firefly effect
const char* bgFragmentShaderSrc = R"(
#version 150 core
in vec2 uv;
out vec4 fragColor;
uniform float loudness;
uniform float time;
uniform sampler2D bgTex;
uniform sampler1D fftTex;

float hash(float n) { return fract(sin(n) * 43758.5453); }
float hash2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123); }

// Simple 1D noise
float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

void main() {
    float fireflyCount = 18.0;
    vec3 fireflyColor = vec3(0.4, 1.0, 0.85);
    vec3 fireflySum = vec3(0.0);
    float maxGlow = 0.0;
    float speedMod = mix(0.5, 2.0, clamp(loudness * 1.5, 0.0, 1.0));
    for (int i = 0; i < int(fireflyCount); ++i) {
        float fi = float(i);
        float baseSeed = fi * 13.37;
        float x0 = hash(baseSeed + 1.0);
        float y0 = hash(baseSeed + 2.0);
        float size = mix(0.07, 0.13, hash(baseSeed + 3.0));
        float speed = mix(0.08, 0.18, hash(baseSeed + 4.0)) * speedMod;
        float twinkle = 0.8 + 0.5 * sin(time * mix(0.7, 1.3, hash(baseSeed + 5.0)) + baseSeed);
        float t = time * speed + baseSeed;
        // Add random walk/noise to the path for more randomness
        float randx = noise(t * 0.5 + baseSeed) * 0.18 - 0.09;
        float randy = noise(t * 0.5 + baseSeed + 100.0) * 0.18 - 0.09;
        float px = x0 + 0.18 * sin(t * 0.23 + fi) + 0.13 * sin(t * 0.11 + fi * 2.1) + randx;
        float py = y0 + 0.18 * cos(t * 0.19 + fi * 1.7) + 0.13 * cos(t * 0.13 + fi * 1.3) + randy;
        vec2 pos = vec2(px, py);
        pos = fract(pos);
        float fftIdx = fract(px) * 0.95 + 0.025;
        float fftVal = texture(fftTex, fftIdx).r;
        float glow = exp(-40.0 * dot(uv - pos, uv - pos) / (size * size));
        float intensity = mix(0.25, 1.0, fftVal) * twinkle * (0.8 + 0.7 * loudness);
        fireflySum += fireflyColor * glow * intensity;
        maxGlow = max(maxGlow, glow * intensity);
    }
    vec2 center = uv - vec2(0.5);
    float cdist = length(center);
    float cglow = exp(-8.0 * cdist * (1.0 - 0.7 * loudness));
    float pulse = 0.7 + 0.3 * sin(time * 1.5 + loudness * 8.0);
    float intensity = cglow * (0.3 + 1.7 * loudness) * pulse;
    vec3 cyan = vec3(0.0, 1.0, 1.0);
    // Mirror horizontally and rotate 180 degrees (flip x, then flip y)
    vec2 final_uv = vec2(uv.x, 1.0 - uv.y);
    vec4 texColor = texture(bgTex, final_uv);
    vec3 blended = texColor.rgb + cyan * intensity + fireflySum;
    fragColor = vec4(blended, 1.0);
}
)";

GLuint bgShader = 0, bgVao = 0, bgVbo = 0;
GLint bgLoudnessLoc = -1, bgTimeLoc = -1, bgTexLoc = -1;

// Add global variable for spectrogram scale
enum class SpectrogramScale { Linear, Log };
SpectrogramScale gSpectrogramScale = SpectrogramScale::Log;

// Add global variable for waveform anchoring
bool gWaveformAnchorZero = false;

// Add global variables for continuous/triggered waveform capture and capture state.
bool gWaveformContinuous = false;
std::atomic<bool> gWaveformCaptureActive{false};
std::atomic<size_t> gWaveformCapturedSamples{0};

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
    // Disabled: the background image is now blended in the shader
    // ImGuiIO& io = ImGui::GetIO();
    // ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    // draw_list->AddImage((void*)(intptr_t)gBackgroundTex, ImVec2(0,0), ImVec2((float)io.DisplaySize.x, (float)io.DisplaySize.y), ImVec2(0,0), ImVec2(1,1));
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
            if (!gWaveformContinuous) {
                gWaveformCaptureActive = true;
                gWaveformCapturedSamples = 0;
                std::lock_guard<std::mutex> lock2(waveformMutex);
                waveformBuffer.clear();
            }
        }
        float sample = models[selected_model_index]->Process();
        out[2 * i] = sample;     // Left channel
        out[2 * i + 1] = sample; // Right channel
        // Store sample for waveform display
        if (gWaveformContinuous) {
            std::lock_guard<std::mutex> lock(waveformMutex);
            waveformBuffer.push_back(sample);
            if (waveformBuffer.size() > WAVEFORM_BUFFER_SIZE) {
                waveformBuffer.pop_front();
            }
        } else {
            if (gWaveformCaptureActive) {
                std::lock_guard<std::mutex> lock(waveformMutex);
                waveformBuffer.push_back(sample);
                gWaveformCapturedSamples++;
                if (gWaveformCapturedSamples >= WAVEFORM_BUFFER_SIZE) {
                    gWaveformCaptureActive = false;
                }
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
            if (ImGui::MenuItem("Quit")) {
                // Set window should close
                GLFWwindow* window = glfwGetCurrentContext();
                if (window) glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            bool isLog = (gSpectrogramScale == SpectrogramScale::Log);
            if (ImGui::MenuItem("Spectrogram: Log Scale", nullptr, isLog)) {
                gSpectrogramScale = SpectrogramScale::Log;
            }
            bool isLinear = (gSpectrogramScale == SpectrogramScale::Linear);
            if (ImGui::MenuItem("Spectrogram: Linear Scale", nullptr, isLinear)) {
                gSpectrogramScale = SpectrogramScale::Linear;
            }
            ImGui::Separator();
            bool anchorZero = gWaveformAnchorZero;
            if (ImGui::MenuItem("Waveform: Anchor at Zero-Crossing", nullptr, anchorZero)) {
                gWaveformAnchorZero = true;
            }
            if (ImGui::MenuItem("Waveform: No Anchor (Raw)", nullptr, !anchorZero)) {
                gWaveformAnchorZero = false;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Waveform: Continuous Capture", nullptr, gWaveformContinuous)) {
                gWaveformContinuous = true;
            }
            if (ImGui::MenuItem("Waveform: Triggered Capture", nullptr, !gWaveformContinuous)) {
                gWaveformContinuous = false;
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
    // --- Improved slider/scrollbar height calculation ---
    float totalSliderHeight = 0.0f;
    constexpr float SAMPLE_RATE = 48000.0f;
    static float timeScale = 1.0f; // seconds, default to 1s
    static float scroll = 0.0f;
    size_t numSamplesToShow = 0;
    size_t maxScroll = 0;
    if (!samples.empty()) {
        totalSliderHeight += ImGui::GetFrameHeightWithSpacing(); // Time Scale always visible if samples exist
        numSamplesToShow = (size_t)(timeScale * SAMPLE_RATE);
        numSamplesToShow = std::min(numSamplesToShow, samples.size());
        maxScroll = samples.size() > numSamplesToShow ? samples.size() - numSamplesToShow : 0;
        if (maxScroll > 0) {
            totalSliderHeight += ImGui::GetFrameHeightWithSpacing(); // Scrollbar only if needed
        }
    }
    ImVec2 plotAvail = avail;
    plotAvail.y = std::max(0.0f, avail.y - totalSliderHeight - 8.0f); // 8px padding
    // --- Time scale and scrollbar additions ---
    if (!samples.empty()) {
        ImGui::SliderFloat("Time Scale (s)", &timeScale, 0.001f, 1.0f, "%.3fs", ImGuiSliderFlags_Logarithmic);
        if (maxScroll > 0) {
            ImGui::SliderFloat("Scroll", &scroll, 0.0f, (float)maxScroll, "%.0f");
        } else {
            scroll = 0.0f;
        }
    }
    size_t scrollIdx = (size_t)scroll;
    if (!samples.empty()) {
        // Always apply anchor and scroll before plotting
        size_t anchorOffset = 0;
        if (gWaveformAnchorZero) {
            for (size_t i = 1; i < samples.size(); ++i) {
                if ((samples[i - 1] <= 0.0f && samples[i] > 0.0f) || (samples[i - 1] >= 0.0f && samples[i] < 0.0f)) {
                    anchorOffset = i;
                    break;
                }
            }
        }
        size_t startIdx = anchorOffset + scrollIdx;
        if (startIdx > samples.size()) startIdx = samples.size();
        size_t endIdx = std::min(startIdx + numSamplesToShow, samples.size());
        int plotCount = (int)(endIdx - startIdx);
        if (plotCount > 0) {
            const float* plotData = samples.data() + startIdx;
            ImGui::PlotLines("Waveform", plotData, plotCount, 0, nullptr, -1.0f, 1.0f, plotAvail);
        } else {
            ImGui::Text("No waveform data.");
        }
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
    // Exponential mapping for y axis (log-frequency)
    size_t nBins = FFT_SIZE/2;
    // Choose mapping based on gSpectrogramScale
    if (gSpectrogramScale == SpectrogramScale::Log) {
        float minFreq = 1.0f; // Avoid log(0)
        float maxFreq = (float)(nBins - 1);
        float logMin = log10(minFreq);
        float logMax = log10(maxFreq);
        for (size_t x = 0; x < WATERFALL_HISTORY; ++x) {
            size_t col = (waterfallPos + WATERFALL_HISTORY - 1 - x) % WATERFALL_HISTORY;
            for (size_t y = 0; y < nBins; ++y) {
                float y_norm = (float)y / (float)(nBins - 1);
                float logF = logMin + y_norm * (logMax - logMin);
                float bin_idx = pow(10.0f, logF);
                if (bin_idx < 0.0f) bin_idx = 0.0f;
                if (bin_idx > maxFreq) bin_idx = maxFreq;
                size_t bin0 = (size_t)bin_idx;
                size_t bin1 = std::min(bin0 + 1, nBins - 1);
                float frac = bin_idx - bin0;
                float v0 = waterfallHistory[col][bin0];
                float v1 = waterfallHistory[col][bin1];
                float v = v0 * (1.0f - frac) + v1 * frac;
                v = std::min(1.0f, v * 20.0f);
                unsigned char c = (unsigned char)(v * 255);
                size_t fy = (nBins - 1) - y;
                size_t idx = 3 * (fy * WATERFALL_HISTORY + x);
                image[idx + 0] = c;
                image[idx + 1] = c;
                image[idx + 2] = c;
            }
        }
    } else { // Linear
        for (size_t x = 0; x < WATERFALL_HISTORY; ++x) {
            size_t col = (waterfallPos + WATERFALL_HISTORY - 1 - x) % WATERFALL_HISTORY;
            for (size_t y = 0; y < nBins; ++y) {
                size_t fy = (nBins - 1) - y;
                float v = std::min(1.0f, waterfallHistory[col][y] * 20.0f);
                unsigned char c = (unsigned char)(v * 255);
                size_t idx = 3 * (fy * WATERFALL_HISTORY + x);
                image[idx + 0] = c;
                image[idx + 1] = c;
                image[idx + 2] = c;
            }
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WATERFALL_HISTORY, nBins, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    } else {
        glBindTexture(GL_TEXTURE_2D, waterfallTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WATERFALL_HISTORY, nBins, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    }
    ImGui::Begin("Waterfall (Spectrogram)", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    // Fill the window completely, even if aspect ratio is not preserved
    ImGui::Image((void*)(intptr_t)waterfallTex, avail);
    ImGui::End();
}

void InitBackgroundShader() {
    GLint status;
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &bgVertexShaderSrc, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &bgFragmentShaderSrc, nullptr);
    glCompileShader(fs);
    bgShader = glCreateProgram();
    glAttachShader(bgShader, vs);
    glAttachShader(bgShader, fs);
    glBindAttribLocation(bgShader, 0, "pos");
    glLinkProgram(bgShader);
    glDeleteShader(vs);
    glDeleteShader(fs);
    float quad[8] = { -1, -1, 1, -1, 1, 1, -1, 1 };
    glGenVertexArrays(1, &bgVao);
    glGenBuffers(1, &bgVbo);
    glBindVertexArray(bgVao);
    glBindBuffer(GL_ARRAY_BUFFER, bgVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindVertexArray(0);
    bgLoudnessLoc = glGetUniformLocation(bgShader, "loudness");
    bgTimeLoc = glGetUniformLocation(bgShader, "time");
    bgTexLoc = glGetUniformLocation(bgShader, "bgTex");
    bgFftTexLoc = glGetUniformLocation(bgShader, "fftTex");
    // Create FFT texture
    glGenTextures(1, &gFftTex);
    glBindTexture(GL_TEXTURE_1D, gFftTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
}

// Helper to upload FFT data to 1D texture
void UploadFftTexture(const std::vector<float>& fftBins) {
    glBindTexture(GL_TEXTURE_1D, gFftTex);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, (GLsizei)fftBins.size(), 0, GL_RED, GL_FLOAT, fftBins.data());
}

float ComputeWaveformLoudness() {
    std::lock_guard<std::mutex> lock(waveformMutex);
    float sum = 0.0f;
    for (float s : waveformBuffer) sum += s * s;
    return waveformBuffer.empty() ? 0.0f : sqrtf(sum / waveformBuffer.size());
}

void RenderAnimatedBackground(float loudness, float time) {
    // Prepare FFT data for shader
    std::vector<float> fftBins(64, 0.0f);
    {
        std::lock_guard<std::mutex> lock(fftMutex);
        if (!fftInputBuffer.empty()) {
            std::vector<float> fftTmp;
            computeFFT(fftInputBuffer, fftTmp);
            // Downsample or average to 64 bins
            for (int i = 0; i < 64; ++i) {
                float sum = 0.0f;
                int start = (int)(i * (fftTmp.size() / 64.0f));
                int end = (int)((i + 1) * (fftTmp.size() / 64.0f));
                for (int j = start; j < end && j < (int)fftTmp.size(); ++j) sum += fftTmp[j];
                fftBins[i] = sum / std::max(1, end - start);
            }
        }
    }
    UploadFftTexture(fftBins);
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(bgShader);
    glUniform1f(bgLoudnessLoc, loudness);
    glUniform1f(bgTimeLoc, time);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBackgroundTex);
    glUniform1i(bgTexLoc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, gFftTex);
    glUniform1i(bgFftTexLoc, 1);
    glBindVertexArray(bgVao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
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

    // Get primary monitor size for maximized window (not fullscreen)
    // Set smaller default window size
    int winWidth = 900;
    int winHeight = 600;
    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "FM Synth", NULL, NULL); // Not fullscreen
    glfwMakeContextCurrent(window);
    // Initialize GLAD after context creation
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD!\n";
        return -1;
    }
    glfwSwapInterval(1);
    // Removed window maximization for smaller default size
    // glfwMaximizeWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    LoadBackgroundTexture();
    InitBackgroundShader();

    // Arrange ImGui windows on first frame
    static bool firstFrame = true;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (firstFrame) {
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2((float)winWidth, (float)winHeight);
            // Arrange windows: Controls left, Waveform center, Waterfall right
            ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(340, winHeight - 60), ImGuiCond_Always);
            ImGui::Begin("FM Drum Synth", nullptr, ImGuiWindowFlags_NoCollapse);
            ImGui::End();
            ImGui::SetNextWindowPos(ImVec2(370, 40), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2((winWidth-370)/2, winHeight/2-50), ImGuiCond_Always);
            ImGui::Begin("Waveform Display", nullptr, ImGuiWindowFlags_NoCollapse);
            ImGui::End();
            ImGui::SetNextWindowPos(ImVec2(370, winHeight/2+10), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2((winWidth-370)/2, winHeight/2-50), ImGuiCond_Always);
            ImGui::Begin("Waterfall (Spectrogram)", nullptr, ImGuiWindowFlags_NoCollapse);
            ImGui::End();
            firstFrame = false;
        }

        // Hotkey: Ctrl+S to save, Ctrl+L to load, Space to trigger
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
        ShowWaveformWindow();
        ShowWaterfallWindow();

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Draw animated background after clearing, before ImGui
        float loudness = ComputeWaveformLoudness();
        float time = (float)glfwGetTime();
        RenderAnimatedBackground(loudness, time);
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
