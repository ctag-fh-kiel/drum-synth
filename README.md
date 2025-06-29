# MD Drum Synth Emulator

This software was mainly generated using AI.

This is a cross-platform, real-time FM drum synthesizer inspired by the Elektron Machinedrum and the EFM (Elektron FM) drum synthesis engine. It provides interactive control and visualization of various FM-based drum models, including Kick, Snare, Tom, Clap, Rimshot, Cowbell, and Cymbal, with a modern GUI and MIDI-like triggering.

## Features
- Real-time FM drum synthesis with multiple classic drum models
- Interactive parameter control via GUI sliders and keyboard (fine/coarse adjustment, navigation)
- Save/load all model parameters to a file (`drum_params.txt`)
- Automatic parameter file creation with sensible defaults
- Cross-platform (tested on macOS, should work on Linux/Windows)

## Controls
- **Up/Down**: Navigate between parameters
- **Left/Right**: Fine parameter adjustment
- **Shift + Left/Right**: Coarse parameter adjustment
- **Space**: Trigger the current drum model
- **Ctrl+S / Ctrl+L**: Save/Load all parameters

## Building

### Prerequisites
- CMake 3.14+
- C++17 compiler (tested on macOS Clang, should work with GCC/MinGW/Visual Studio)
- [RtAudio](https://github.com/thestk/rtaudio) (fetched automatically)
- [GLFW](https://github.com/glfw/glfw) (fetched automatically)
- [Dear ImGui](https://github.com/ocornut/imgui) (fetched automatically)

### Build Steps
```sh
mkdir build && cd build
cmake ..
cmake --build .
./fm_drum_synth
```

### Notes
- On first run, a `drum_params.txt` file will be created with default settings if it does not exist.
- The background image is embedded at compile time from `resources/background.png` (see `resources/README.md` for details).

## References
- EFM (Elektron FM) Synthesis: [Elektronauts forum post with EFM paper and diagrams](https://www.elektronauts.com/t/md-voices-diagram/173460/16)
- [Elektron Machinedrum Voices Diagram (Elektronauts forum)](https://www.elektronauts.com/t/md-voices-diagram/173460/16)
- This implementation is inspired by the EFM algorithms described in the original Elektron documentation and community research.

---

For more information, see the code comments and the referenced Elektron/EFM paper.
