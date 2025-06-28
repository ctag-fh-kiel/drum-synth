#pragma once
#include "DrumModel.h"

class TRXSnareDrum : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;

    void saveParameters(std::ostream& os) const override {
        os << pitch << ' ' << decay << ' ' << snap << ' ' << noise
           << ' ' << tone << ' ' << tune << ' ' << bump << ' ' << clip << '\n';
    }

    void loadParameters(std::istream& is) override {
        is >> pitch >> decay >> snap >> noise >> tone >> tune >> bump >> clip;
    }

private:
    // Parameters
    float pitch = 180.0f;      // Base pitch (Hz)
    float decay = 0.4f;        // Amplitude decay
    float snap = 0.6f;         // Extra noisy attack
    float noise = 0.5f;        // Noise level
    float tone = 0.5f;         // Balance between oscillators
    float tune = 100.0f;       // Frequency interval between osc1 and osc2
    float bump = 0.1f;         // Small pitch rise at start
    float clip = 0.2f;         // Clipping intensity

    // Envelope
    float t = 0.0f;
    float ampEnv = 0.0f;
    float snapEnv = 0.0f;

    // Oscillator phase
    float phase1 = 0.0f;
    float phase2 = 0.0f;

    // Filter state for noise
    float hp_x = 0.0f, hp_y = 0.0f;

    float sine(float x);
};
