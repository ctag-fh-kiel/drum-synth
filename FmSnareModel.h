// FmSnareModel.h
#pragma once
#include "DrumModel.h"

class FmSnareModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;
    void saveParameters(std::ostream& os) const override {
        os << f_b << ' ' << d_b << ' ' << f_m << ' ' << I << ' ' << d_m << ' ' << Abrus << '\n';
    }
    void loadParameters(std::istream& is) override {
        is >> f_b >> d_b >> f_m >> I >> d_m >> Abrus;
    }

private:
    float f_b = 200.0f, d_b = 0.4f, f_m = 1500.0f, I = 15.0f, d_m = 0.1f, Abrus = 0.5f;
    float mod_phase = 0.0f, car_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};