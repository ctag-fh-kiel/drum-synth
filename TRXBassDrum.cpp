#include "TRXBassDrum.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

constexpr float kSampleRate = 48000.0f;

void TRXBassDrum::Init() {
    phase = t = env = rampEnv = 0.0f;
    prevSample = 0.0f;
}

void TRXBassDrum::Trigger() {
    t = 0.0f;
    env = 1.0f;
    rampEnv = 1.0f;
    phase = 0.0f;
}

float TRXBassDrum::Process() {
    if (env <= 0.0001f) return 0.0f;

    t += 1.0f / kSampleRate;

    // Envelope decay
    env *= std::exp(-1.0f / (decay * kSampleRate));
    rampEnv *= std::exp(-1.0f / (rampDecay * kSampleRate));

    // Frequency modulation
    float freq = pitch + ramp * rampEnv * 1000.0f;
    phase += freq / kSampleRate;
    if (phase > 1.0f) phase -= 1.0f;

    float sineOut = sine(phase * 2.0f * M_PI);
    float value = sineOut * env * start;

    // Add harmonic distortion
    if (harmonics > 0.0f) {
        value += harmonics * std::tanh(sineOut * 3.0f) * env;
    }

    // Add noise burst
    if (noise > 0.0f && t < 0.01f) {
        value += noise * ((rand() / (float)RAND_MAX) * 2.0f - 1.0f) * env;
    }

    // Soft clip
    if (clip > 0.0f) {
        value = std::tanh(value * (1.0f + clip * 5.0f));
    }

    return value;
}

void TRXBassDrum::RenderControls() {
    ImGui::SliderFloat("Pitch", &pitch, 20.0f, 120.0f);
    ImGui::SliderFloat("Decay", &decay, 0.01f, 2.0f);
    ImGui::SliderFloat("Ramp", &ramp, 0.0f, 1.0f);
    ImGui::SliderFloat("Ramp Decay", &rampDecay, 0.01f, 1.0f);
    ImGui::SliderFloat("Start", &start, 0.0f, 2.0f);
    ImGui::SliderFloat("Noise", &noise, 0.0f, 1.0f);
    ImGui::SliderFloat("Harmonics", &harmonics, 0.0f, 1.0f);
    ImGui::SliderFloat("Clip", &clip, 0.0f, 1.0f);
}

float TRXBassDrum::sine(float x) {
    return std::sin(x); // Replace with lookup if performance becomes a concern
}
