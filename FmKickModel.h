#pragma once
#include "DrumModel.h"
#include "mi/operator.h"

class FmKickModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;
    void saveParameters(std::ostream& os) const override {
        os << f_b << ' ' << d_b << ' ' << f_m << ' ' << I << ' ' << d_m << ' ' << b_m << ' ' << A_f << ' ' << d_f << '\n';
    }
    void loadParameters(std::istream& is) override {
        is >> f_b >> d_b >> f_m >> I >> d_m >> b_m >> A_f >> d_f;
    }

private:
    float f_b = 50.0f, d_b = 0.5f, f_m = 180.0f, I = 20.0f;
    float d_m = 0.15f, b_m = 0.5f, A_f = 60.0f, d_f = 0.1f;

    // Iterative decay state
    float amp_env = 1.0f, mod_env = 1.0f, freq_env = 1.0f;
    float amp_decay_const = 0.0f, mod_decay_const = 0.0f, freq_decay_const = 0.0f;

    // Plaits FM operator state
    plaits::fm::Operator ops[2]; // [0]=modulator, [1]=carrier
    float fb_state[2] = {0.0f, 0.0f};
    float t = 0.0f;
};
