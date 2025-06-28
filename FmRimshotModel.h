// FmRimshotModel.h
#pragma once
#include "DrumModel.h"

class FmRimshotModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;
    void saveParameters(std::ostream& os) const override {
        os << f_b1 << ' ' << d_b1 << ' ' << f_m1 << ' ' << I1 << ' '
           << f_b2 << ' ' << d_b2 << ' ' << f_m2 << ' ' << I2 << ' '
           << d_m << ' ' << A2 << ' ' << f_hp << '\n';
    }

    void loadParameters(std::istream& is) override {
        is >> f_b1 >> d_b1 >> f_m1 >> I1
           >> f_b2 >> d_b2 >> f_m2 >> I2
           >> d_m >> A2 >> f_hp;
    }
private:
    // Rim hit FM pair
    float f_b1 = 600.0f, f_m1 = 1800.0f, I1 = 15.0f, d_b1 = 0.05f;

    // Body FM pair
    float f_b2 = 200.0f, f_m2 = 600.0f, I2 = 10.0f, d_b2 = 0.25f;

    // Shared mod envelope
    float d_m = 0.05f;

    // Mix and filter
    float A2 = 0.4f;
    float f_hp = 400.0f;

    // Phase and state
    float t = 0.0f;
    float car1_phase = 0.0f, mod1_phase = 0.0f;
    float car2_phase = 0.0f, mod2_phase = 0.0f;
    float prev_mod1 = 0.0f, prev_mod2 = 0.0f;
    float x_prev = 0.0f, y_prev = 0.0f;
};



