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

void FmKickModel::Init() {
    t = 0.0f;
    // Reset iterative decays
    amp_env = 1.0f;
    mod_env = 1.0f;
    freq_env = 1.0f;
    // Reset Plaits operator state
    ops[0].Reset();
    ops[1].Reset();
    fb_state[0] = 0.0f;
    fb_state[1] = 0.0f;
}

void FmKickModel::Trigger() {
    Init();
    // Calculate decay constants for iterative envelopes WITHOUT std::expf
    float dt = 1.0f / SAMPLE_RATE;
    // For small x, exp(-x) ≈ 1 - x
    amp_decay_const = 1.0f - (dt / d_b);
    mod_decay_const = 1.0f - (dt / d_m);
    freq_decay_const = 1.0f - (dt / d_f);
    // Clamp to [0,1] to avoid negative decay in pathological cases
    if (amp_decay_const < 0.0f) amp_decay_const = 0.0f;
    if (mod_decay_const < 0.0f) mod_decay_const = 0.0f;
    if (freq_decay_const < 0.0f) freq_decay_const = 0.0f;
}

float FmKickModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    t += dt;
    // Iterative decay
    amp_env *= amp_decay_const;
    mod_env *= mod_decay_const;
    freq_env *= freq_decay_const;
    float freq_env_scaled = A_f * freq_env;

    // Prepare Plaits FM operator parameters
    float f[2];
    float a[2];
    // Modulator frequency selection
    float mod_freq = f_m;
    if (use_ratio_mode) {
        mod_freq = f_b * (ratios[ratio_index][0] / ratios[ratio_index][1]);
    }
    f[0] = mod_freq / SAMPLE_RATE; // modulator frequency (normalized)
    f[1] = (f_b + freq_env_scaled) / SAMPLE_RATE; // carrier frequency (normalized)
    a[0] = I * mod_env; // modulator amplitude (mod index)
    a[1] = amp_env;     // carrier amplitude

    float out = 0.0f;
    // Feedback amount for modulator (0-7)
    int fb_amt = static_cast<int>(b_m);
    // Render a single sample using Plaits FM operator (2-op, modulator feeds carrier)
    plaits::fm::RenderOperators<2, 0, false>(
        ops, f, a, fb_state, fb_amt, nullptr, &out, 1);
    return out;
}

void FmKickModel::RenderControls() {
    // Carrier frequency (pitch of the drum)
    CustomControls::ParameterSlider("f_b (Base Frequency)", &f_b, 20.0f, 100.0f);

    // UI: Ratio mode toggle
    ImGui::Checkbox("Lock Modulator to Ratio", &use_ratio_mode);
    if (use_ratio_mode) {
        static const char* ratio_labels[num_ratios] = {
            "1:1", "2:1", "3:2", "3:1", "4:1", "5:2", "5:1", "7:4", "7:2", "9:2", "15:8", "13:8"
        };
        ImGui::Combo("Modulator Ratio", &ratio_index, ratio_labels, num_ratios);
    } else {
        // Modulator frequency (determines harmonic complexity)
        CustomControls::ParameterSlider("f_m (Modulator Freq)", &f_m, 50.0f, 2000.0f);
    }

    // Volume envelope decay (controls how long the drum rings out)
    CustomControls::ParameterSlider("d_b (Amp Decay)", &d_b, 0.01f, 2.0f);

    // Modulation index (depth of FM, sharpness of attack)
    CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 10.0f, 0.001f, 0.01f);

    // Modulator envelope decay (shorter = clickier attack)
    CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.001f, 2.0f, 0.001f, 0.01f);

    // Feedback on the modulator (adds noise/grit to tone)
    CustomControls::ParameterSlider("b_m (Mod Feedback)", &b_m, .0f, 16.0f, 1, 2);

    // Frequency sweep amount (in Hz)
    CustomControls::ParameterSlider("A_f (Freq Sweep Amt)", &A_f, 0.0f, 1000.0f);

    // Frequency envelope decay (how fast pitch sweep drops)
    CustomControls::ParameterSlider("d_f (Freq Sweep Decay)", &d_f, 0.001f, 2.0f, 0.001f, 0.01f  );
}