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
    for (int i = 0; i < NUM_PAIRS; ++i) {
        car_phase[i] = mod_phase[i] = PI / 2.0f;
        prev_mod[i] = 0.0f;
    }
    x_prev = y_prev = 0.0f;
}

void FmCymbalModel::Trigger() {
    Init();
}

float FmCymbalModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    float amp_env = sustain + ExpDecay(t, d_b);
    float mod_env = ExpDecay(t, d_m);

    float ratios[NUM_PAIRS] = {1.0f, 1.411f, 1.8f, 2.7f};
    float sample = 0.0f;

    for (int i = 0; i < NUM_PAIRS; ++i) {
        mod_phase[i] = WrapPhase(mod_phase[i] + TWO_PI * (fm * ratios[i]) * dt + bb * prev_mod[i]);
        float mod_out = std::sin(mod_phase[i]);
        prev_mod[i] = mod_out;

        car_phase[i] = WrapPhase(car_phase[i] + TWO_PI * (fb * ratios[i]) * dt + I * mod_env * mod_out);
        float car_out = std::sin(car_phase[i]);

        sample += car_out;
    }

    float mixed = sample * 0.25f * amp_env;

    float alpha = 1.0f / (1.0f + 2.0f * PI * f_hp * dt);
    float y = alpha * (y_prev + mixed - x_prev);
    x_prev = mixed;
    y_prev = y;

    t += dt;
    return y;
}

void FmCymbalModel::RenderControls() {
    CustomControls::ParameterSlider("fb (Base Carrier)", &fb, 100.0f, 1000.0f);
    CustomControls::ParameterSlider("fm (Base Mod)", &fm, 200.0f, 2000.0f);
    CustomControls::ParameterSlider("d_b (Amp Decay)", &d_b, 0.05f, 4.0f);
    CustomControls::ParameterSlider("I (FM Index)", &I, 0.0f, 30.0f);
    CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.05f, 2.0f);
    CustomControls::ParameterSlider("bb (Mod Feedback)", &bb, 0.0f, 1.0f);
    CustomControls::ParameterSlider("sustain", &sustain, 0.0f, 1.0f);
    CustomControls::ParameterSlider("f_hp (HPF Cutoff)", &f_hp, 100.0f, 2000.0f);
}