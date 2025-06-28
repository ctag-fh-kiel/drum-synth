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
        os << fb << ' ' << fm << ' ' << d_b << ' ' << I << ' '
           << d_m << ' ' << bb << ' ' << sustain << ' ' << f_hp << '\n';
    }

    void loadParameters(std::istream& is) override {
        is >> fb >> fm >> d_b >> I >> d_m >> bb >> sustain >> f_hp;
    }

private:
    static constexpr int NUM_PAIRS = 4;
    float fb = 400.0f;     // base carrier frequency
    float fm = 800.0f;     // base modulator frequency
    float d_b = 1.0f;      // amp decay
    float I = 10.0f;       // FM index
    float d_m = 0.2f;      // mod env decay
    float bb = 0.5f;       // mod feedback
    float sustain = 0.3f;  // constant bias
    float f_hp = 300.0f;   // high-pass filter

    float car_phase[NUM_PAIRS] = {};
    float mod_phase[NUM_PAIRS] = {};
    float prev_mod[NUM_PAIRS] = {};
    float t = 0.0f;
    float x_prev = 0.0f, y_prev = 0.0f;
};
