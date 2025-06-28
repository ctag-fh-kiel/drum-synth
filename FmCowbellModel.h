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
        os << fbA << ' ' << fbB << ' ' << d_b1 << ' ' << db2 << ' ' << fm << ' ' << I << ' ' << dm << ' ' << bm << ' ' << Ab1 << ' ' << Ab2 << '\n';
    }
    void loadParameters(std::istream& is) override {
        is >> fbA >> fbB >> d_b1 >> db2 >> fm >> I >> dm >> bm >> Ab1 >> Ab2;
    }

private:
    float fbA = 540.0f, fbB, d_b1 = 0.015f, db2 = 0.1f, fm = 2000.0f;
    float I = 15.0f, dm = 0.1f, bm = 0.3f, Ab1 = 0.7f, Ab2;
    float mod_phase = 0.0f, carA_phase = 0.0f, carB_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};
