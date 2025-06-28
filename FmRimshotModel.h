// FmRimshotModel.h
#pragma once
#include "DrumModel.h"

class FmRimshotModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;

private:
    float f_b1 = 400.0f, d_b1 = 0.1f, I1 = 15.0f;
    float f_b2 = 200.0f, d_b2 = 0.3f, I2 = 10.0f;
    float A2 = 0.4f;
    float t = 0.0f;
};