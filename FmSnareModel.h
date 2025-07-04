// FmSnareModel.h
#pragma once
#include "DrumModel.h"
#include "mi/operator.h"

class FmSnareModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;
    void saveParameters(std::ostream& os) const override {
        os << f_b << ' ' << d_b << ' ' << f_m << ' ' << I << ' ' << d_m << ' ' << Abrus << ' ' << dbrus << ' ' << fhp << '\n';
    }
    void loadParameters(std::istream& is) override {
        is >> f_b >> d_b >> f_m >> I >> d_m >> Abrus >> dbrus >> fhp;
    }

private:
    // FM parameters
    float f_b = 200.0f;     // Carrier frequency
    float d_b = 0.4f;       // Amplitude envelope decay
    float f_m = 1500.0f;    // Modulator frequency
    float I = 15.0f;        // Modulation index
    float d_m = 0.1f;       // Modulator envelope decay

    // Noise and filter
    float Abrus = 0.5f;     // Noise level
    float dbrus = 0.3f;     // Noise envelope decay
    float fhp = 400.0f;     // High-pass filter cutoff (Hz)

    // Internal state
    float t = 0.0f;
    float y_prev = 0.0f, x_prev = 0.0f; // HPF state

    // Envelope states and decay constants
    float amp_env = 1.0f;
    float mod_env = 1.0f;
    float noise_env = 1.0f;
    float amp_decay_const = 0.0f;
    float mod_decay_const = 0.0f;
    float noise_decay_const = 0.0f;

    // Mutable Instruments FM operators
    plaits::fm::Operator modulator_;
    plaits::fm::Operator carrier_;
    float fb_state_[2] = {0.0f, 0.0f};
};
