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
    car1_phase = car2_phase = mod1_phase = mod2_phase = PI / 2.0f;
    prev_mod1 = prev_mod2 = 0.0f;
    x_prev = y_prev = 0.0f;
}

void FmRimshotModel::Trigger() {
    Init();
}

float FmRimshotModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;

    float env1 = ExpDecay(t, d_b1);
    float env2 = ExpDecay(t, d_b2);
    float mod_env = ExpDecay(t, d_m);

    // Rim pair
    mod1_phase = WrapPhase(mod1_phase + TWO_PI * f_m1 * dt + 0.5f * prev_mod1);
    float mod1_out = std::sin(mod1_phase);
    prev_mod1 = mod1_out;

    car1_phase = WrapPhase(car1_phase + TWO_PI * f_b1 * dt + I1 * mod_env * mod1_out);
    float tone1 = std::sin(car1_phase) * env1;

    // Body pair
    mod2_phase = WrapPhase(mod2_phase + TWO_PI * f_m2 * dt + 0.5f * prev_mod2);
    float mod2_out = std::sin(mod2_phase);
    prev_mod2 = mod2_out;

    car2_phase = WrapPhase(car2_phase + TWO_PI * f_b2 * dt + I2 * mod_env * mod2_out);
    float tone2 = std::sin(car2_phase) * env2;

    // Mix and filter
    float mixed = (1.0f - A2) * tone1 + A2 * tone2;
    float alpha = 1.0f / (1.0f + 2.0f * PI * f_hp * dt);
    float y = alpha * (y_prev + mixed - x_prev);

    x_prev = mixed;
    y_prev = y;
    t += dt;

    return y;
}

void FmRimshotModel::RenderControls() {
    CustomControls::ParameterSlider("f_b1 (rim freq)", &f_b1, 200.0f, 1000.0f);
    CustomControls::ParameterSlider("d_b1 (rim decay)", &d_b1, 0.01f, 0.5f);
    CustomControls::ParameterSlider("f_m1", &f_m1, 200.0f, 3000.0f);
    CustomControls::ParameterSlider("I1", &I1, 0.0f, 50.0f);

    CustomControls::ParameterSlider("f_b2 (body freq)", &f_b2, 80.0f, 400.0f);
    CustomControls::ParameterSlider("d_b2 (body decay)", &d_b2, 0.05f, 1.0f);
    CustomControls::ParameterSlider("f_m2", &f_m2, 100.0f, 2000.0f);
    CustomControls::ParameterSlider("I2", &I2, 0.0f, 50.0f);

    CustomControls::ParameterSlider("d_m (mod env decay)", &d_m, 0.01f, 0.5f);
    CustomControls::ParameterSlider("A2 (body mix)", &A2, 0.0f, 1.0f);
    CustomControls::ParameterSlider("f_hp (HPF cutoff)", &f_hp, 100.0f, 2000.0f);
}
