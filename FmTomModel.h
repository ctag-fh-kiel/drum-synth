// FmTomModel.h
#pragma once
#include "DrumModel.h"

class FmTomModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;

private:
    float f_b = 150.0f, d_b = 0.7f, f_m = 300.0f, I = 15.0f, d_m = 0.2f;
    float A_f = 30.0f, d_f = 0.1f, start_phase = 3.14159f / 2.0f;
    float mod_phase = 0.0f, car_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};