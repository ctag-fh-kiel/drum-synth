// FmCowbellModel.cpp
#include "FmCowbellModel.h"
#include "CustomControls.h"
#include <cmath>
#include <imgui.h>

constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float SAMPLE_RATE = 48000.0f;

static float WrapPhase(float phase) {
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0.0f) phase += TWO_PI;
    return phase;
}

static float ExpDecay(float t, float decay_time) {
    return std::expf(-t / decay_time);
}

void FmCowbellModel::Init() {
    t = 0.0f;
    mod_phase = 0.0f;
    carA_phase = carB_phase = PI / 2.0f;
    prev_mod = 0.0f;
    fbB = fbA * 1.48f;
    Ab2 = 1.0f - Ab1;
}

void FmCowbellModel::Trigger() {
    Init();
}

float FmCowbellModel::Process() {
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

void FmCowbellModel::RenderControls() {
    CustomControls::ParameterSlider("fbA", &fbA, 200.0f, 1000.0f);
    CustomControls::ParameterSlider("d_b1", &d_b1, 0.005f, 0.2f);
    CustomControls::ParameterSlider("db2", &db2, 0.01f, 1.0f);
    CustomControls::ParameterSlider("fm", &fm, 500.0f, 3000.0f);
    CustomControls::ParameterSlider("I", &I, 0.0f, 100.0f);
    CustomControls::ParameterSlider("dm", &dm, 0.01f, 1.0f);
    CustomControls::ParameterSlider("bm", &bm, 0.0f, 1.0f);
    CustomControls::ParameterSlider("Ab1", &Ab1, 0.0f, 1.0f);
}
