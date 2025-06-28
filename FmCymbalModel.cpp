// FmCymbalModel.cpp
#include "FmCymbalModel.h"
#include "CustomControls.h"
#include <cmath>

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

void FmCymbalModel::Init() {
    t = 0.0f;
    float base = 400.0f;
    fb[0] = base;
    fb[1] = base * 1.411f;
    fb[2] = base * 1.8f;
    fb[3] = base * 2.7f;

    fm[0] = base * 2.0f;
    fm[1] = fm[0] * 1.411f;
    fm[2] = fm[0] * 1.8f;
    fm[3] = fm[0] * 2.7f;

    for (int i = 0; i < NUM_PAIRS * 2; ++i)
        phases[i] = PI / 2.0f;
}

void FmCymbalModel::Trigger() {
    Init();
}

float FmCymbalModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    float amp_env = std::fmin(1.0f, sustain + ExpDecay(t, d_b));
    float mod_env = ExpDecay(t, d_m);
    float sample = 0.0f;

    for (int i = 0; i < NUM_PAIRS; ++i) {
        int mod_i = i * 2;
        int car_i = i * 2 + 1;

        float mod_out = std::sin(phases[mod_i]);
        phases[mod_i] = WrapPhase(phases[mod_i] + TWO_PI * fm[i] * dt + bb * mod_out);

        float car_out = std::sin(phases[car_i] + I * mod_env * mod_out);
        phases[car_i] = WrapPhase(phases[car_i] + TWO_PI * fb[i] * dt);

        sample += car_out;
    }

    t += dt;
    return amp_env * sample * 0.25f; // normalize sum of 4 pairs
}

void FmCymbalModel::RenderControls() {
    CustomControls::ParameterSlider("d_b", &d_b, 0.05f, 4.0f);
    CustomControls::ParameterSlider("I", &I, 0.0f, 30.0f);
    CustomControls::ParameterSlider("d_m", &d_m, 0.05f, 2.0f);
    CustomControls::ParameterSlider("bb", &bb, 0.0f, 1.0f);
    CustomControls::ParameterSlider("sustain", &sustain, 0.0f, 1.0f);
}
