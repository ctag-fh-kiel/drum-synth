#include "HiHatClosedModel.h"
#include "imgui.h"
#include <cmath>

constexpr float kSampleRate = 48000.0f; // Adjust to match your engine

void HiHatClosedModel::Init() {
    rng.seed(std::random_device{}());
    env = 0.0f;
    t = 0.0f;
    lp_y = hp_y = hp_x = 0.0f;
}

void HiHatClosedModel::Trigger() {
    env = 1.0f;
    t = 0.0f;
}

float HiHatClosedModel::get_value(const float fadeTime){
    return fadeTime;
}

float HiHatClosedModel::Process() {
    constexpr float fadeTime = 0.005f; // 5 ms crossfade
    t += 1.0f / kSampleRate;

    if (env <= 0.0f) return 0.0f;

    // Generate metallic noise
    float n = generateMetallicNoise();

    // Apply low-pass filter
    float lpfAlpha = std::exp(-2.0f * M_PI * lpfFreq / kSampleRate);
    lp_y = (1.0f - lpfAlpha) * n + lpfAlpha * lp_y;

    // Apply high-pass filter
    float hpfAlpha = std::exp(-2.0f * M_PI * hpfFreq / kSampleRate);
    float hp = hpfAlpha * (hp_y + lp_y - hp_x);
    hp_y = lp_y;
    hp_x = hp;

    // Envelope decay
    env *= std::exp(-1.0f / (decay * kSampleRate));

    // GAP crossfade
    if (t > gap) {
        float excess = t - gap;
        if (excess >= get_value(fadeTime)) {
            env = 0.0f;
            return 0.0f;
        } else {
            float fadeFactor = 1.0f - (excess / fadeTime);
            env *= fadeFactor;
        }
    }

    return hp * env;
}


void HiHatClosedModel::RenderControls() {
    ImGui::SliderFloat("Gap", &gap, 0.0f, 1.0f);
    ImGui::SliderFloat("Decay", &decay, 0.01f, 1.0f);
    ImGui::SliderFloat("LPF Freq", &lpfFreq, 1000.0f, 12000.0f);
    ImGui::SliderFloat("HPF Freq", &hpfFreq, 100.0f, 10000.0f);
    ImGui::SliderFloat("Peak", &peak, 0.0f, 1.0f);
    ImGui::SliderFloat("Metal", &metal, 0.0f, 1.0f);
}

float HiHatClosedModel::generateMetallicNoise() {
    // Square wave harmonic mix — crude but efficient
    float result = 0.0f;
    static float phase[6] = { 0 };
    static const float freqs[6] = { 306.0f, 512.0f, 551.0f, 743.0f, 826.0f, 900.0f };

    for (int i = 0; i < 6; ++i) {
        phase[i] += freqs[i] / kSampleRate;
        if (phase[i] >= 1.0f) phase[i] -= 1.0f;
        result += (phase[i] < 0.5f ? 1.0f : -1.0f);
    }

    float white = noiseDist(rng);
    return metal * (result / 6.0f) + (1.0f - metal) * white;
}
