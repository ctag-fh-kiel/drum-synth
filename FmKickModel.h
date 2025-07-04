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
        os << f_b << ' ' << d_b << ' ' << f_m << ' ' << I << ' ' << d_m << ' ' << b_m << ' ' << A_f << ' ' << d_f << ' ' << use_ratio_mode << ' ' << ratio_index << ' ' << mod_env_sync << '\n';
    }
    void loadParameters(std::istream& is) override {
        is >> f_b >> d_b >> f_m >> I >> d_m >> b_m >> A_f >> d_f >> use_ratio_mode >> ratio_index >> mod_env_sync;
    }

private:
    float f_b = 50.0f, d_b = 0.5f, f_m = 180.0f, I = 20.0f;
    float d_m = 0.15f, b_m = 0.5f, A_f = 60.0f, d_f = 0.1f;

    // Ratio mode for modulator frequency
    bool use_ratio_mode = false;
    int ratio_index = 0; // Index into ratio array
    static constexpr int num_ratios = 12;
    static constexpr float ratios[num_ratios][2] = {
        {1.0f, 1.0f},   // 1:1
        {2.0f, 1.0f},   // 2:1
        {3.0f, 2.0f},   // 3:2
        {3.0f, 1.0f},   // 3:1
        {4.0f, 1.0f},   // 4:1
        {5.0f, 2.0f},   // 5:2
        {5.0f, 1.0f},   // 5:1
        {7.0f, 4.0f},   // 7:4
        {7.0f, 2.0f},   // 7:2
        {9.0f, 2.0f},   // 9:2
        {15.0f, 8.0f},  // 15:8
        {13.0f, 8.0f}   // 13:8
    };

    // Iterative decay state
    float amp_env = 1.0f, mod_env = 1.0f, freq_env = 1.0f;
    float amp_decay_const = 0.0f, mod_decay_const = 0.0f, freq_decay_const = 0.0f;

    // Plaits FM operator state
    plaits::fm::Operator ops[2]; // [0]=modulator, [1]=carrier
    float fb_state[2] = {0.0f, 0.0f};
    float t = 0.0f;

    bool mod_env_sync = false; // New: sync modulator freq envelope to carrier
};
