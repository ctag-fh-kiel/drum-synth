// FmRimshotModel.cpp
#include "FmRimshotModel.h"
#include "CustomControls.h"
#include <cmath>

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

void FmRimshotModel::Init() {
    t = 0.0f;
    carB_phase = carA_phase = mod_phase = PI / 2.0f;
    prev_mod = x_prev = y_prev = 0.0f;
}

void FmRimshotModel::Trigger() {
    Init();
}

float FmRimshotModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;

    float mod_env = ExpDecay(t, d_m);
    float mod_out = std::sin(mod_phase);
    mod_phase = WrapPhase(mod_phase + TWO_PI * 1000.0f * dt);  // fixed mod freq
    prev_mod = mod_out;

    float envB = ExpDecay(t, d_bB);
    float carB = std::sin(WrapPhase(carB_phase + I_B * mod_env * mod_out));
    carB_phase = WrapPhase(carB_phase + TWO_PI * f_bB * dt);

    float envA = ExpDecay(t, d_bA);
    float carA = std::sin(WrapPhase(carA_phase + I_A * mod_env * mod_out));
    carA_phase = WrapPhase(carA_phase + TWO_PI * f_bA * dt);

    float mixed = (1.0f - A_A) * (carB * envB) + A_A * (carA * envA);

    float alpha = 1.0f / (1.0f + 2.0f * PI * f_hp * dt);
    float y = alpha * (y_prev + mixed - x_prev);
    x_prev = mixed;
    y_prev = y;

    t += dt;
    return y;
}

void FmRimshotModel::RenderControls() {
    CustomControls::ParameterSlider("f_bB (rim freq)", &f_bB, 200.0f, 1000.0f);
    CustomControls::ParameterSlider("d_bB (rim decay)", &d_bB, 0.01f, 0.5f);
    CustomControls::ParameterSlider("I_B (rim mod index)", &I_B, 0.0f, 50.0f);

    CustomControls::ParameterSlider("f_bA (body freq)", &f_bA, 80.0f, 400.0f);
    CustomControls::ParameterSlider("d_bA (body decay)", &d_bA, 0.05f, 1.0f);
    CustomControls::ParameterSlider("I_A (body mod index)", &I_A, 0.0f, 50.0f);

    CustomControls::ParameterSlider("A_A (body mix)", &A_A, 0.0f, 1.0f);
    CustomControls::ParameterSlider("d_m (mod env decay)", &d_m, 0.01f, 0.5f);
    CustomControls::ParameterSlider("f_hp (HPF cutoff)", &f_hp, 100.0f, 2000.0f);
}
