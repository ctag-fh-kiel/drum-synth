// main.cpp
#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <RtAudio.h>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float SAMPLE_RATE = 48000.0f;
constexpr size_t BUFFER_SIZE = 256;

float WrapPhase(float phase) {
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0.0f) phase += TWO_PI;
    return phase;
}

float ExpDecay(float t, float decay_time) {
    return std::expf(-t / decay_time);
}

enum class ModelType { KICK, COWBELL };
std::atomic<ModelType> current_model(ModelType::KICK);
std::atomic<bool> trigger_requested(false);
std::mutex param_mutex;

class FmKick {
public:
    void Init() { mod_phase = car_phase = t = prev_mod = 0; }

    void Trigger(float f_b_, float d_b_, float f_m_, float I_, float d_m_, float b_m_, float A_f_, float d_f_) {
        f_b = f_b_; d_b = d_b_; f_m = f_m_; I = I_; d_m = d_m_;
        b_m = b_m_; A_f = A_f_; d_f = d_f_;
        t = 0.0f; mod_phase = 0.0f; car_phase = PI / 2.0f;
    }

    float Process() {
        float dt = 1.0f / SAMPLE_RATE;
        float amp_env = ExpDecay(t, d_b);
        float mod_env = ExpDecay(t, d_m);
        float freq_env = A_f * ExpDecay(t, d_f);

        float mod_feedback = b_m * prev_mod;
        mod_phase = WrapPhase(mod_phase + TWO_PI * f_m * dt + mod_feedback);
        float mod_out = std::sin(mod_phase);
        prev_mod = mod_out;

        car_phase = WrapPhase(car_phase + TWO_PI * (f_b + freq_env) * dt + I * mod_env * mod_out);
        float out = std::sin(car_phase) * amp_env;

        t += dt;
        return out;
    }

private:
    float f_b, d_b, f_m, I, d_m, b_m, A_f, d_f;
    float mod_phase = 0.0f, car_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};

class FmCowbell {
public:
    void Init() {
        t = 0;
        mod_phase = 0;
        carA_phase = carB_phase = PI / 2.0f;
        prev_mod = 0;
    }

    void Trigger(float fbA_, float d_b1_, float f_m_, float I_, float d_m_, float b_m_, float Ab1_, float db2_) {
        fbA = fbA_;
        fbB = fbA * 1.48f;
        d_b1 = d_b1_;
        db2 = db2_;
        fm = f_m_;
        I = I_;
        dm = d_m_;
        bm = b_m_;
        Ab1 = Ab1_;
        Ab2 = 1.0f - Ab1_;
        t = 0;
        mod_phase = 0;
        carA_phase = carB_phase = PI / 2.0f;
        prev_mod = 0;
    }

    float Process() {
        float dt = 1.0f / SAMPLE_RATE;
        float env1 = ExpDecay(t, d_b1);
        float env2 = ExpDecay(t, db2);
        float amp_env = Ab1 * env1 + Ab2 * env2;

        float mod_env = ExpDecay(t, dm);
        float mod_feedback = bm * prev_mod;
        mod_phase = WrapPhase(mod_phase + TWO_PI * fm * dt + mod_feedback);
        float mod_out = std::sin(mod_phase);
        prev_mod = mod_out;

        float mod_signal = I * mod_env * mod_out;

        carA_phase = WrapPhase(carA_phase + TWO_PI * fbA * dt + mod_signal);
        carB_phase = WrapPhase(carB_phase + TWO_PI * fbB * dt + mod_signal);

        float outA = std::sin(carA_phase);
        float outB = std::sin(carB_phase);
        float out = (outA + outB) * 0.5f * amp_env;

        t += dt;
        return out;
    }

private:
    float fbA, fbB, d_b1, db2, fm, I, dm, bm, Ab1, Ab2;
    float mod_phase = 0.0f, carA_phase = 0.0f, carB_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};

struct KickParams {
    float f_b = 50.0f, d_b = 0.5f, f_m = 180.0f, I = 20.0f;
    float d_m = 0.15f, b_m = 0.5f, A_f = 60.0f, d_f = 0.1f;
} kick_params;

struct CowbellParams {
    float fbA = 540.0f, d_b1 = 0.015f, fm = 2000.0f, I = 15.0f;
    float d_m = 0.1f, b_m = 0.3f, Ab1 = 0.7f, db2 = 0.15f;
} cowbell_params;

FmKick kick;
FmCowbell cowbell;

int audioCallback(void* outputBuffer, void*, unsigned int nBufferFrames, double, RtAudioStreamStatus, void*) {
    float* out = reinterpret_cast<float*>(outputBuffer);

    for (unsigned int i = 0; i < nBufferFrames; ++i) {
        if (trigger_requested.exchange(false)) {
            std::lock_guard<std::mutex> lock(param_mutex);
            if (current_model == ModelType::KICK)
                kick.Trigger(kick_params.f_b, kick_params.d_b, kick_params.f_m, kick_params.I, kick_params.d_m, kick_params.b_m, kick_params.A_f, kick_params.d_f);
            else
                cowbell.Trigger(cowbell_params.fbA, cowbell_params.d_b1, cowbell_params.fm, cowbell_params.I, cowbell_params.d_m, cowbell_params.b_m, cowbell_params.Ab1, cowbell_params.db2);
        }
        out[i] = (current_model == ModelType::KICK) ? kick.Process() : cowbell.Process();
    }
    return 0;
}

void ShowControls() {
    ImGui::Begin("FM Synth Controls");

    int model = (current_model == ModelType::KICK) ? 0 : 1;
    if (ImGui::RadioButton("Kick", model == 0)) current_model = ModelType::KICK;
    ImGui::SameLine();
    if (ImGui::RadioButton("Cowbell", model == 1)) current_model = ModelType::COWBELL;

    if (ImGui::Button("Trigger")) trigger_requested = true;

    if (current_model == ModelType::KICK) {
        ImGui::SliderFloat("f_b", &kick_params.f_b, 20.0f, 100.0f);
        ImGui::SliderFloat("d_b", &kick_params.d_b, 0.01f, 2.0f);
        ImGui::SliderFloat("f_m", &kick_params.f_m, 50.0f, 1000.0f);
        ImGui::SliderFloat("I",   &kick_params.I, 0.0f, 50.0f);
        ImGui::SliderFloat("d_m", &kick_params.d_m, 0.01f, 2.0f);
        ImGui::SliderFloat("b_m", &kick_params.b_m, 0.0f, 1.0f);
        ImGui::SliderFloat("A_f", &kick_params.A_f, 0.0f, 200.0f);
        ImGui::SliderFloat("d_f", &kick_params.d_f, 0.01f, 2.0f);
    } else {
        ImGui::SliderFloat("fbA", &cowbell_params.fbA, 100.0f, 1000.0f);
        ImGui::SliderFloat("d_b1", &cowbell_params.d_b1, 0.001f, 0.5f);
        ImGui::SliderFloat("fm", &cowbell_params.fm, 500.0f, 4000.0f);
        ImGui::SliderFloat("I", &cowbell_params.I, 0.0f, 50.0f);
        ImGui::SliderFloat("d_m", &cowbell_params.d_m, 0.01f, 2.0f);
        ImGui::SliderFloat("b_m", &cowbell_params.b_m, 0.0f, 1.0f);
        ImGui::SliderFloat("Ab1", &cowbell_params.Ab1, 0.0f, 1.0f);
        ImGui::SliderFloat("db2", &cowbell_params.db2, 0.01f, 2.0f);
    }

    ImGui::End();
}

int main() {
    kick.Init();
    cowbell.Init();

    RtAudio dac;
    if (dac.getDeviceCount() < 1) {
        std::cerr << "No audio devices found!\n";
        return 1;
    }

    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 1;
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
    ImGui_ImplOpenGL3_Init("#version 150"); // Use GLSL 150 (OpenGL 3.2)

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        std::lock_guard<std::mutex> lock(param_mutex);
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
