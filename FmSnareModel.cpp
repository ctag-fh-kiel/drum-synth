// FmSnareModel.cpp
#include "FmSnareModel.h"
#include "CustomControls.h"
#include "mi/operator.h"
#include <cmath>
#include <cstdlib>
#include <imgui.h>

constexpr float SAMPLE_RATE = 48000.0f;
constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;

void FmSnareModel::Init() {
    t = 0.0f;
    modulator_.Reset();
    carrier_.Reset();
    x_prev = y_prev = 0.0f;
    fb_state_[0] = fb_state_[1] = 0.0f;
    amp_env = 1.0f;
    mod_env = 1.0f;
    noise_env = 1.0f;
    // Calculate decay constants for iterative envelopes WITHOUT std::expf
    float dt = 1.0f / SAMPLE_RATE;
    amp_decay_const = 1.0f - (dt / d_b);
    mod_decay_const = 1.0f - (dt / d_m);
    noise_decay_const = 1.0f - (dt / dbrus);
    if (amp_decay_const < 0.0f) amp_decay_const = 0.0f;
    if (mod_decay_const < 0.0f) mod_decay_const = 0.0f;
    if (noise_decay_const < 0.0f) noise_decay_const = 0.0f;
}

void FmSnareModel::Trigger() {
    Init();
}

float FmSnareModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    // Iterative envelope decay
    amp_env *= amp_decay_const;
    mod_env *= mod_decay_const;
    noise_env *= noise_decay_const;

    // Prepare frequency and amplitude for operators (normalized to [0, 0.5] for Nyquist)
    float mod_freq = f_m / SAMPLE_RATE;
    float car_freq = f_b / SAMPLE_RATE;
    float mod_amp = I * mod_env; // Modulation index as amplitude
    float car_amp = amp_env;

    float mod_out = 0.0f;
    float car_out = 0.0f;
    // Render modulator (no feedback, no external modulation)
    plaits::fm::Operator* mod_ops[1] = { &modulator_ };
    float mod_f[1] = { mod_freq };
    float mod_a[1] = { mod_amp };
    float dummy_fb[2] = {0.0f, 0.0f};
    float dummy_mod[1] = {0.0f};
    float mod_buf[1] = {0.0f};
    plaits::fm::RenderOperators<1, plaits::fm::Operator::MODULATION_SOURCE_NONE, false>(
        mod_ops[0], mod_f, mod_a, dummy_fb, 0, dummy_mod, mod_buf, 1);
    mod_out = mod_buf[0];

    // Render carrier (external modulation from modulator)
    plaits::fm::Operator* car_ops[1] = { &carrier_ };
    float car_f[1] = { car_freq };
    float car_a[1] = { car_amp };
    float car_mod[1] = { mod_out };
    float car_buf[1] = {0.0f};
    plaits::fm::RenderOperators<1, plaits::fm::Operator::MODULATION_SOURCE_EXTERNAL, false>(
        car_ops[0], car_f, car_a, dummy_fb, 0, car_mod, car_buf, 1);
    float tone = car_buf[0];

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
