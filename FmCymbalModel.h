// FmCymbalModel.h
#pragma once
#include "DrumModel.h"

class FmCymbalModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;
    void saveParameters(std::ostream& os) const override {
        for (int i = 0; i < NUM_PAIRS; ++i) os << fb[i] << ' ' << fm[i] << ' ';
        os << d_b << ' ' << I << ' ' << d_m << ' ' << bb << ' ' << sustain << ' ' << fhp << '\n';
    }
    void loadParameters(std::istream& is) override {
        for (int i = 0; i < NUM_PAIRS; ++i) is >> fb[i] >> fm[i];
        is >> d_b >> I >> d_m >> bb >> sustain >> fhp;
    }

private:
    static constexpr int NUM_PAIRS = 4;
    float fb[NUM_PAIRS], fm[NUM_PAIRS];
    float phases[NUM_PAIRS * 2] = {};
    float d_b = 1.0f, I = 10.0f, d_m = 0.2f, bb = 0.5f, sustain = 0.3f, fhp = 300.0f;
    float t = 0.0f;
};