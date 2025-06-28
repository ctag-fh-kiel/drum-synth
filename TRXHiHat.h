#pragma once
#include "DrumModel.h"
#include <array>
#include <random>

class TRXHiHat : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float get_value(float fadeTime);
    float Process() override;
    void RenderControls() override;

    void saveParameters(std::ostream& os) const override {
        os << gap << ' ' << decay << ' ' << lpfFreq << ' '
           << hpfFreq << ' ' << peak << ' ' << metal << '\n';
    }

    void loadParameters(std::istream& is) override {
        is >> gap >> decay >> lpfFreq >> hpfFreq >> peak >> metal;
    }

private:
    // Parameters
    float gap = 0.5f;
    float decay = 0.2f;
    float lpfFreq = 8000.0f;
    float hpfFreq = 4000.0f;
    float peak = 0.5f;
    float metal = 0.7f;

    // Envelope
    float env = 0.0f;
    float t = 0.0f;

    // Filters
    float lp_y = 0.0f;
    float hp_y = 0.0f;
    float hp_x = 0.0f;

    // Noise source
    std::default_random_engine rng;
    std::uniform_real_distribution<float> noiseDist{-1.0f, 1.0f};

    float generateMetallicNoise();
};
