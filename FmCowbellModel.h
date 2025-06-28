// FmCowbellModel.h
#pragma once
#include "DrumModel.h"

class FmCowbellModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;

    void saveParameters(std::ostream& os) const override {
        os << fbA << ' ' << d_b1 << ' ' << db2 << ' ' << fm << ' '
           << I << ' ' << dm << ' ' << bm << ' ' << Ab1 << '\n';
    }

    void loadParameters(std::istream& is) override {
        is >> fbA >> d_b1 >> db2 >> fm >> I >> dm >> bm >> Ab1;
        fbB = fbA * 1.48f;
        Ab2 = 1.0f - Ab1;
    }

private:
    float fbA = 540.0f, fbB = 540.0f * 1.48f;
    float d_b1 = 0.015f, db2 = 0.1f;
    float fm = 2000.0f, I = 15.0f, dm = 0.1f, bm = 0.3f;
    float Ab1 = 0.7f, Ab2 = 1.0f - 0.7f;

    float mod_phase = 0.0f, carA_phase = 0.0f, carB_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};
