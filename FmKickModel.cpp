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
    // Carrier frequency (pitch of the drum)
    CustomControls::ParameterSlider("f_b (Base Frequency)", &f_b, 20.0f, 100.0f);

    // Volume envelope decay (controls how long the drum rings out)
    CustomControls::ParameterSlider("d_b (Amp Decay)", &d_b, 0.01f, 2.0f);

    // Modulator frequency (determines harmonic complexity)
    CustomControls::ParameterSlider("f_m (Modulator Freq)", &f_m, 50.0f, 1000.0f);

    // Modulation index (depth of FM, sharpness of attack)
    CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 50.0f);

    // Modulator envelope decay (shorter = clickier attack)
    CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.01f, 2.0f);

    // Feedback on the modulator (adds noise/grit to tone)
    CustomControls::ParameterSlider("b_m (Mod Feedback)", &b_m, 0.0f, 1.0f);

    // Frequency sweep amount (in Hz)
    CustomControls::ParameterSlider("A_f (Freq Sweep Amt)", &A_f, 0.0f, 200.0f);

    // Frequency envelope decay (how fast pitch sweep drops)
    CustomControls::ParameterSlider("d_f (Freq Sweep Decay)", &d_f, 0.01f, 2.0f);
}