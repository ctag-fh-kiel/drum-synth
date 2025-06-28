// main.cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <atomic>
#include <thread>
#include <mutex>
#include <RtAudio.h>
#include <sstream>

constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float SAMPLE_RATE = 48000.0f;
constexpr size_t BUFFER_SIZE = 256;

float WrapPhase(float phase) {
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0.0f) phase += TWO_PI;
    return phase;
}

float ExpDecay(float t, float decay_time) {
    return std::expf(-t / decay_time);
}

class FmKick {
public:
    void Init() {
        mod_phase = car_phase = t = 0;
        prev_mod = 0;
    }

    void Trigger(float f_b_, float d_b_, float f_m_, float I_, float d_m_, float b_m_, float A_f_, float d_f_) {
        f_b = f_b_; d_b = d_b_; f_m = f_m_; I = I_; d_m = d_m_;
        b_m = b_m_; A_f = A_f_; d_f = d_f_;
        t = 0.0f; mod_phase = 0.0f; car_phase = PI / 2.0f; // Cosine start
    }

    float Process() {
        float dt = 1.0f / SAMPLE_RATE;
        float amp_env = ExpDecay(t, d_b);
        float mod_env = ExpDecay(t, d_m);
        float freq_env = A_f * ExpDecay(t, d_f);

        float mod_feedback = b_m * prev_mod;
        mod_phase = WrapPhase(mod_phase + TWO_PI * f_m * dt + mod_feedback);
        float mod_out = std::sin(mod_phase);
        prev_mod = mod_out;

        car_phase = WrapPhase(car_phase + TWO_PI * (f_b + freq_env) * dt + I * mod_env * mod_out);
        float out = std::sin(car_phase) * amp_env;

        t += dt;
        return out;
    }

private:
    float f_b, d_b, f_m, I, d_m, b_m, A_f, d_f;
    float mod_phase = 0.0f, car_phase = 0.0f, prev_mod = 0.0f, t = 0.0f;
};

std::mutex param_mutex;
std::atomic<bool> trigger_requested(false);

// Shared parameters
struct KickParams {
    float f_b = 50.0f;
    float d_b = 0.5f;
    float f_m = 180.0f;
    float I = 20.0f;
    float d_m = 0.15f;
    float b_m = 0.5f;
    float A_f = 60.0f;
    float d_f = 0.1f;
} params;

FmKick kick;

int audioCallback(void* outputBuffer, void*, unsigned int nBufferFrames,
                  double, RtAudioStreamStatus, void*) {
    float* out = reinterpret_cast<float*>(outputBuffer);

    for (unsigned int i = 0; i < nBufferFrames; ++i) {
        if (trigger_requested.exchange(false)) {
            std::lock_guard<std::mutex> lock(param_mutex);
            kick.Trigger(params.f_b, params.d_b, params.f_m, params.I,
                         params.d_m, params.b_m, params.A_f, params.d_f);
        }
        out[i] = kick.Process();
    }
    return 0;
}

void UIThread() {
    std::string cmd;
    while (true) {
        std::cout << "\nType 't' to trigger or 'set [param] [value]': ";
        std::getline(std::cin, cmd);
        if (cmd == "t") {
            trigger_requested = true;
        } else if (cmd.rfind("set", 0) == 0) {
            std::istringstream iss(cmd);
            std::string _, name;
            float value;
            iss >> _ >> name >> value;
            std::lock_guard<std::mutex> lock(param_mutex);
            if (name == "f_b") params.f_b = value;
            else if (name == "d_b") params.d_b = value;
            else if (name == "f_m") params.f_m = value;
            else if (name == "I") params.I = value;
            else if (name == "d_m") params.d_m = value;
            else if (name == "b_m") params.b_m = value;
            else if (name == "A_f") params.A_f = value;
            else if (name == "d_f") params.d_f = value;
            else std::cout << "Unknown parameter\n";
        }
    }
}

int main() {
    kick.Init();

    RtAudio dac;
    if (dac.getDeviceCount() < 1) {
        std::cerr << "No audio devices found!\n";
        return 1;
    }

    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 1;
    parameters.firstChannel = 0;

    unsigned int bufferFrames = BUFFER_SIZE;


    dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32, SAMPLE_RATE, &bufferFrames, &audioCallback, nullptr);
    dac.startStream();


    std::thread ui_thread(UIThread);
    ui_thread.join();

    return 0;
}
