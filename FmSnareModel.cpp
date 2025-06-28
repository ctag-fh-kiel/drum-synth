// FmSnareModel.cpp
#include "FmSnareModel.h"
#include <cmath>
#include <cstdlib>
#include <imgui.h>

constexpr float SAMPLE_RATE = 48000.0f;
constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;

static float WrapPhase(float phase) {
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0.0f) phase += TWO_PI;
    return phase;
}

static float ExpDecay(float t, float decay_time) {
    return std::expf(-t / decay_time);
}

void FmSnareModel::Init() {
    t = 0.0f;
    mod_phase = car_phase = PI / 2.0f;
    prev_mod = 0.0f;
}

void FmSnareModel::Trigger() {
    Init();
}

float FmSnareModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    float amp_env = ExpDecay(t, d_b);
    float mod_env = ExpDecay(t, d_m);

    float mod_feedback = 1.0f * prev_mod; // High feedback for noise
    mod_phase = WrapPhase(mod_phase + TWO_PI * f_m * dt + mod_feedback);
    float mod_out = std::sin(mod_phase);
    prev_mod = mod_out;

    car_phase = WrapPhase(car_phase + TWO_PI * f_b * dt + I * mod_env * mod_out);
    float tone = std::sin(car_phase);
    float noise = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f);
    float out = (tone + Abrus * noise) * amp_env;

    t += dt;
    return out;
}

void FmSnareModel::RenderControls() {
    ImGui::SliderFloat("f_b", &f_b, 100.0f, 400.0f);
    ImGui::SliderFloat("d_b", &d_b, 0.01f, 1.0f);
    ImGui::SliderFloat("f_m", &f_m, 500.0f, 3000.0f);
    ImGui::SliderFloat("I", &I, 0.0f, 50.0f);
    ImGui::SliderFloat("d_m", &d_m, 0.01f, 1.0f);
    ImGui::SliderFloat("Abrus", &Abrus, 0.0f, 1.0f);
}
