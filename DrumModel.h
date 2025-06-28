#pragma once

#include <iostream>

class DrumModel {
public:
    virtual ~DrumModel() {}
    virtual void Init() = 0;
    virtual void Trigger() = 0;
    virtual float Process() = 0;
    virtual void RenderControls() = 0;

    // Serialization interface for saving/loading parameters
    virtual void saveParameters(std::ostream& os) const = 0;
    virtual void loadParameters(std::istream& is) = 0;
};