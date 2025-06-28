#pragma once

#include <string>
#include <vector>
#include <functional>

namespace CustomControls {

struct ParamInfo {
    std::string label;
    float* value_ptr;
    float min_val, max_val;
    float step, fast_step;
};

void BeginParameters();
void EndParameters();
void ParameterSlider(const char* label, float* v, float v_min, float v_max, float step = 0.01f, float fast_step = 0.1f);

}

