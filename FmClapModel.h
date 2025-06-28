// FmClapModel.h
#pragma once
#include "DrumModel.h"

class FmClapModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;
    void saveParameters(std::ostream& os) const override {
        os << f_b << ' ' << f_m << ' ' << I << ' ' << d_m << ' ' << d1 << ' ' << d2 << ' ' << clap_count << '\n';
    }
    void loadParameters(std::istream& is) override {
        is >> f_b >> f_m >> I >> d_m >> d1 >> d2 >> clap_count;
    }

private:
    float f_b = 800.0f, f_m = 800.0f, I = 40.0f, d_m = 0.05f;
    float d1 = 0.02f, d2 = 0.3f;
    int clap_count = 3, clap_stage = 0;
    float mod_phase = 0.0f, car_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};