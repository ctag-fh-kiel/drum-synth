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
    bool is_int = false;
    int* int_ptr = nullptr;
    int int_min = 0, int_max = 0, int_step = 1, int_fast_step = 1;
};

void BeginParameters();
void EndParameters();
void ParameterSlider(const char* label, float* v, float v_min, float v_max, float step = 0.01f, float fast_step = 0.1f);
void ParameterSliderInt(const char* label, int* v, int v_min, int v_max, int step = 1, int fast_step = 1);

}
