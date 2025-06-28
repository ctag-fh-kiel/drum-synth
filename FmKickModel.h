#pragma once
#include "DrumModel.h"

class FmKickModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;

private:
    float f_b = 50.0f, d_b = 0.5f, f_m = 180.0f, I = 20.0f;
    float d_m = 0.15f, b_m = 0.5f, A_f = 60.0f, d_f = 0.1f;
    float mod_phase = 0.0f, car_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};
