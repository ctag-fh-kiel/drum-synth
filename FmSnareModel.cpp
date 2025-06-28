// FmSnareModel.cpp
#include "FmSnareModel.h"
#include "CustomControls.h"
#include <cmath>
#include <cstdlib>
#include <imgui.h>

constexpr float SAMPLE_RATE = 48000.0f;
constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;

float WrapPhase(float phase) {
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0.0f) phase += TWO_PI;
    return phase;
}

float ExpDecay(float t, float decay_time) {
    return std::expf(-t / decay_time);
}

void FmSnareModel::Init() {
    t = 0.0f;
    mod_phase = car_phase = PI / 2.0f;
    prev_mod = 0.0f;
    x_prev = y_prev = 0.0f;
}

void FmSnareModel::Trigger() {
    Init();
}

float FmSnareModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    float amp_env = ExpDecay(t, d_b);
    float mod_env = ExpDecay(t, d_m);
    float noise_env = ExpDecay(t, dbrus);

    float mod_feedback = 1.0f * prev_mod;
    mod_phase = WrapPhase(mod_phase + TWO_PI * f_m * dt + mod_feedback);
    float mod_out = std::sin(mod_phase);
    prev_mod = mod_out;

    car_phase = WrapPhase(car_phase + TWO_PI * f_b * dt + I * mod_env * mod_out);
    float tone = std::sin(car_phase);

    float white = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * Abrus * noise_env;

    float x = tone + white;
    float alpha = 1.0f / (1.0f + 2.0f * PI * fhp * dt);
    float y = alpha * (y_prev + x - x_prev);

    x_prev = x;
    y_prev = y;
    t += dt;

    return y * amp_env;
}

void FmSnareModel::RenderControls() {
    CustomControls::ParameterSlider("f_b (Tone Freq)", &f_b, 100.0f, 400.0f);
    CustomControls::ParameterSlider("d_b (Tone Decay)", &d_b, 0.01f, 1.0f);
    CustomControls::ParameterSlider("f_m (Mod Freq)", &f_m, 500.0f, 3000.0f);
    CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 50.0f);
    CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.01f, 1.0f);
    CustomControls::ParameterSlider("Abrus (Noise Level)", &Abrus, 0.0f, 1.0f);
    CustomControls::ParameterSlider("dbrus (Noise Decay)", &dbrus, 0.01f, 1.0f);
    CustomControls::ParameterSlider("fhp (HPF Cutoff)", &fhp, 20.0f, 2000.0f);
}
