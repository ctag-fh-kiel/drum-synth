#pragma once

class DrumModel {
public:
    virtual ~DrumModel() {}
    virtual void Init() = 0;
    virtual void Trigger() = 0;
    virtual float Process() = 0;
    virtual void RenderControls() = 0;
};