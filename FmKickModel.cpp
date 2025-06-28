#include "FmKickModel.h"
#include <cmath>
#include <imgui.h>
#include "CustomControls.h"

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

void FmKickModel::Init() {
    t = 0.0f;
    mod_phase = 0.0f;
    car_phase = PI / 2.0f;
    prev_mod = 0.0f;
}

void FmKickModel::Trigger() {
    Init();
}

float FmKickModel::Process() {
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

void FmKickModel::RenderControls() {
    CustomControls::ParameterSlider("f_b", &f_b, 20.0f, 100.0f);
    CustomControls::ParameterSlider("d_b", &d_b, 0.01f, 2.0f);
    CustomControls::ParameterSlider("f_m", &f_m, 50.0f, 1000.0f);
    CustomControls::ParameterSlider("I", &I, 0.0f, 50.0f);
    CustomControls::ParameterSlider("d_m", &d_m, 0.01f, 2.0f);
    CustomControls::ParameterSlider("b_m", &b_m, 0.0f, 1.0f);
    CustomControls::ParameterSlider("A_f", &A_f, 0.0f, 200.0f);
    CustomControls::ParameterSlider("d_f", &d_f, 0.01f, 2.0f);
}
