#include "TRXSnareDrum.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

constexpr float kSampleRate = 48000.0f;

void TRXSnareDrum::Init() {
    t = ampEnv = snapEnv = 0.0f;
    phase1 = phase2 = 0.0f;
    hp_x = hp_y = 0.0f;
}

void TRXSnareDrum::Trigger() {
    t = 0.0f;
    ampEnv = 1.0f;
    snapEnv = 1.0f;
    phase1 = phase2 = 0.0f;
}

float TRXSnareDrum::Process() {
    if (ampEnv <= 0.0001f) return 0.0f;

    t += 1.0f / kSampleRate;

    // Decay envelopes
    ampEnv *= std::exp(-1.0f / (decay * kSampleRate));
    snapEnv *= std::exp(-1.0f / (0.02f * kSampleRate)); // 20ms snap noise decay

    // Oscillators (tuned with interval)
    float freq1 = pitch + bump * 80.0f;
    float freq2 = pitch + tune;

    phase1 += freq1 / kSampleRate;
    if (phase1 > 1.0f) phase1 -= 1.0f;
    float osc1 = sine(phase1 * 2.0f * M_PI);

    phase2 += freq2 / kSampleRate;
    if (phase2 > 1.0f) phase2 -= 1.0f;
    float osc2 = sine(phase2 * 2.0f * M_PI);

    float tonePart = (tone * osc1 + (1.0f - tone) * osc2) * ampEnv;

    // Snap noise burst
    float snapNoise = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * snap * snapEnv;

    // Sustained filtered noise (high-pass)
    float rawNoise = ((rand() / (float)RAND_MAX) * 2.0f - 1.0f);
    float hp_a = std::exp(-2.0f * M_PI * 400.0f / kSampleRate);
    float hp = hp_a * (hp_y + rawNoise - hp_x);
    hp_y = rawNoise;
    hp_x = hp;

    float noisePart = noise * hp * ampEnv;

    // Sum
    float out = tonePart + snapNoise + noisePart;

    // Clip
    if (clip > 0.0f) {
        out = std::tanh(out * (1.0f + clip * 5.0f));
    }

    return out;
}

void TRXSnareDrum::RenderControls() {
    ImGui::SliderFloat("Pitch", &pitch, 60.0f, 400.0f);
    ImGui::SliderFloat("Decay", &decay, 0.05f, 1.0f);
    ImGui::SliderFloat("Snap", &snap, 0.0f, 1.0f);
    ImGui::SliderFloat("Noise", &noise, 0.0f, 1.0f);
    ImGui::SliderFloat("Tone Balance", &tone, 0.0f, 1.0f);
    ImGui::SliderFloat("Tune Interval", &tune, 0.0f, 400.0f);
    ImGui::SliderFloat("Bump", &bump, 0.0f, 1.0f);
    ImGui::SliderFloat("Clip", &clip, 0.0f, 1.0f);
}

float TRXSnareDrum::sine(float x) {
    return std::sin(x);
}
