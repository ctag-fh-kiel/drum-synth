#pragma once
#include "DrumModel.h"

class TRXBassDrum : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void RenderControls() override;

    void saveParameters(std::ostream& os) const override {
        os << pitch << ' ' << decay << ' ' << ramp << ' ' << rampDecay << ' '
           << start << ' ' << noise << ' ' << harmonics << ' ' << clip << '\n';
    }

    void loadParameters(std::istream& is) override {
        is >> pitch >> decay >> ramp >> rampDecay >> start >> noise >> harmonics >> clip;
    }

private:
    // User parameters
    float pitch = 50.0f;       // Base pitch in Hz
    float decay = 0.4f;        // Envelope decay time
    float ramp = 0.3f;         // Frequency ramp amount
    float rampDecay = 0.1f;    // Ramp decay time
    float start = 1.0f;        // Start amplitude multiplier
    float noise = 0.0f;        // Noise at attack
    float harmonics = 0.0f;    // Adds clipped harmonic content
    float clip = 0.0f;         // Soft clipping amount

    // Internal state
    float phase = 0.0f;
    float t = 0.0f;
    float env = 0.0f;
    float rampEnv = 0.0f;
    float prevSample = 0.0f;

    // Helpers
    float sine(float x);
};
