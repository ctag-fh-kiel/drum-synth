#include "TRXClaves.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

constexpr float kSampleRate = 48000.0f;

void TRXClaves::Init() {
    env = 0.0f;
    t = 0.0f;
    phase1 = phase2 = 0.0f;
}

void TRXClaves::Trigger() {
    env = 1.0f;
    t = 0.0f;
    phase1 = phase2 = 0.0f;
}

float TRXClaves::Process() {
    if (env < 0.0001f) return 0.0f;

    t += 1.0f / kSampleRate;
    env *= std::exp(-1.0f / (decay * kSampleRate));

    phase1 += pitch / kSampleRate;
    phase2 += (pitch + interval) / kSampleRate;
    if (phase1 > 1.0f) phase1 -= 1.0f;
    if (phase2 > 1.0f) phase2 -= 1.0f;

    float osc1 = sine(phase1 * 2.0f * M_PI);
    float osc2 = sine(phase2 * 2.0f * M_PI);
    float mix = balance * osc1 + (1.0f - balance) * osc2;
    float out = mix * env;

    if (clip > 0.0f) {
        out = std::tanh(out * (1.0f + clip * 5.0f));
    }

    return out;
}

void TRXClaves::RenderControls() {
    ImGui::SliderFloat("Pitch", &pitch, 200.0f, 4000.0f);
    ImGui::SliderFloat("Interval", &interval, 0.0f, 400.0f);
    ImGui::SliderFloat("Decay", &decay, 0.01f, 0.5f);
    ImGui::SliderFloat("Balance", &balance, 0.0f, 1.0f);
    ImGui::SliderFloat("Clip", &clip, 0.0f, 1.0f);
}

float TRXClaves::sine(float x) {
    return std::sin(x);
}
