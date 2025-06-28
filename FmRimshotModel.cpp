// FmRimshotModel.cpp
#include "FmRimshotModel.h"
#include <cmath>
#include <imgui.h>

constexpr float SAMPLE_RATE = 48000.0f;
static float ExpDecay(float t, float decay_time) {
    return std::expf(-t / decay_time);
}

void FmRimshotModel::Init() {
    t = 0.0f;
}

void FmRimshotModel::Trigger() {
    Init();
}

float FmRimshotModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    float env1 = ExpDecay(t, d_b1);
    float env2 = ExpDecay(t, d_b2);

    float signal1 = std::sin(2.0f * 3.14159f * f_b1 * t) * I1 * env1;
    float signal2 = std::sin(2.0f * 3.14159f * f_b2 * t) * I2 * env2;

    float out = (1.0f - A2) * signal1 + A2 * signal2;
    t += dt;
    return out;
}

void FmRimshotModel::RenderControls() {
    ImGui::SliderFloat("f_b1 (rim)", &f_b1, 100.0f, 1000.0f);
    ImGui::SliderFloat("d_b1 (rim decay)", &d_b1, 0.01f, 1.0f);
    ImGui::SliderFloat("I1 (rim tone)", &I1, 0.0f, 50.0f);
    ImGui::SliderFloat("f_b2 (drum)", &f_b2, 100.0f, 1000.0f);
    ImGui::SliderFloat("d_b2 (drum decay)", &d_b2, 0.01f, 2.0f);
    ImGui::SliderFloat("I2 (drum tone)", &I2, 0.0f, 50.0f);
    ImGui::SliderFloat("A2 (mix)", &A2, 0.0f, 1.0f);
}
