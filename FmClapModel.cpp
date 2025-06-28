// FmClapModel.cpp
#include "FmClapModel.h"
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

void FmClapModel::Init() {
    t = 0.0f;
    clap_stage = 0;
    clap_timer = 0.0f;
    mod_phase = car_phase = PI / 2.0f;
    prev_mod = 0.0f;
    x_prev = y_prev = 0.0f;
    active = true;
}

void FmClapModel::Trigger() {
    Init();
}

float FmClapModel::Process() {
    if (!active) return 0.0f;

    float dt = 1.0f / SAMPLE_RATE;
    float decay = (clap_stage < clap_count) ? d1 : d2;
    float amp_env = ExpDecay(t, decay);

    float mod_env = ExpDecay(t, d_m);
    float mod_feedback = prev_mod;
    mod_phase = WrapPhase(mod_phase + TWO_PI * f_m * dt + mod_feedback);
    float mod_out = std::sin(mod_phase);
    prev_mod = mod_out;

    car_phase = WrapPhase(car_phase + TWO_PI * f_b * dt + I * mod_env * mod_out);
    float tone = std::sin(car_phase);
    float x = tone * amp_env;

    float alpha = 1.0f / (1.0f + 2.0f * PI * fhp * dt);
    float y = alpha * (y_prev + x - x_prev);
    x_prev = x;
    y_prev = y;

    t += dt;
    clap_timer += dt;

    if (clap_timer >= clap_interval) {
        ++clap_stage;
        t = 0.0f;
        clap_timer = 0.0f;
        if (clap_stage >= clap_count + 1)
            active = false;
    }

    return y;
}

void FmClapModel::RenderControls() {
    CustomControls::ParameterSlider("f_b (Base Freq)", &f_b, 400.0f, 1200.0f);
    CustomControls::ParameterSlider("f_m (Mod Freq)", &f_m, 100.0f, 3000.0f);
    CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 100.0f);
    CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.01f, 1.0f);
    CustomControls::ParameterSlider("d1 (Pre-Clap Decay)", &d1, 0.005f, 0.2f);
    CustomControls::ParameterSlider("d2 (Final Clap Decay)", &d2, 0.01f, 0.6f);
    CustomControls::ParameterSliderInt("clap_count", &clap_count, 1, 6);
    CustomControls::ParameterSlider("fhp (High-Pass Cutoff)", &fhp, 20.0f, 2000.0f);
}