#include "CustomControls.h"
#include "imgui.h"
#include <vector>

namespace CustomControls {

static std::vector<ParamInfo> g_Params;
static int g_SelectedIndex = -1;

void BeginParameters() {
    g_Params.clear();
}

void EndParameters() {
    if (g_Params.empty()) return;

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
        if (g_SelectedIndex > 0) {
            g_SelectedIndex--;
        } else if (g_SelectedIndex == -1 && !g_Params.empty()) {
            g_SelectedIndex = 0;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
        if (g_SelectedIndex < (int)g_Params.size() - 1) {
            g_SelectedIndex++;
        } else if (g_SelectedIndex == -1 && !g_Params.empty()) {
            g_SelectedIndex = 0;
        }
    }

    if (g_SelectedIndex != -1) {
        ParamInfo& param = g_Params[g_SelectedIndex];

        if (param.is_int && param.int_ptr) {
            int step = io.KeyShift ? param.int_fast_step : param.int_step;

            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
                *param.int_ptr -= step;
                if (*param.int_ptr < param.int_min) *param.int_ptr = param.int_min;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
                *param.int_ptr += step;
                if (*param.int_ptr > param.int_max) *param.int_ptr = param.int_max;
            }
        } else if (param.value_ptr) {
            float step = io.KeyShift ? param.fast_step : param.step;

            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
                *param.value_ptr -= step;
                if (*param.value_ptr < param.min_val) *param.value_ptr = param.min_val;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
                *param.value_ptr += step;
                if (*param.value_ptr > param.max_val) *param.value_ptr = param.max_val;
            }
        }
    }
}

void ParameterSlider(const char* label, float* v, float v_min, float v_max, float step, float fast_step) {
    g_Params.push_back({label, v, v_min, v_max, step, fast_step, false});
    int current_index = g_Params.size() - 1;

    bool is_selected = (g_SelectedIndex == current_index);
    if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0.6f, 0.8f, 0.8f));
    }

    ImGui::SliderFloat(label, v, v_min, v_max);

    if (is_selected) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked()) {
        g_SelectedIndex = current_index;
    }
}

void ParameterSliderInt(const char* label, int* v, int v_min, int v_max, int step, int fast_step) {
    g_Params.push_back({label, nullptr, 0, 0, 0, 0, true, v, v_min, v_max, step, fast_step});
    int current_index = g_Params.size() - 1;

    bool is_selected = (g_SelectedIndex == current_index);
    if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0.1f, 0.8f, 0.8f));
    }

    ImGui::SliderInt(label, v, v_min, v_max);

    if (is_selected) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked()) {
        g_SelectedIndex = current_index;
    }
}

}
