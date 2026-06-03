# Additive Synthesizer

A polyphonic additive synthesizer VST3/AU plugin built with JUCE and CMake. Synthesizes audio by summing up to 512 sine-wave partials per voice with full spectral, envelope, and filter control.

## Credits

The additive synthesizer (`projects/AdditiveSynthesizer/` and `projects/DSP/AddSynth/`) was implemented by me as part of the **Audio Programming Project by Aalto University** course. The supporting framework (`mrta_utils/`, `cmake/`, `configure.sh`, `build.sh`) was provided by the course.

## Features

- **8-voice polyphony** with per-voice additive synthesis
- **Up to 512 partials** per voice (configurable at runtime)
- **Spectral shaping** — manual partial amplitudes, classic waveform presets (Sine, Saw, Square, Triangle), and sampled harmonic tables (Flute, Trumpet C, Violin)
- **Inharmonicity, Offset, Stretch, and Odd/Even Balance** controls for spectral morphing
- **Amplitude ADSR envelope**
- **State Variable Filter** with its own ADSR envelope and LFO
- Formats: **VST3 · AU · Standalone**
- Platform: **macOS** (universal arm64/x86_64), Windows, Linux

## Parameters

| Section | Parameter | Range |
|---|---|---|
| Partials | Num Partials | 1 – 512 |
| | Shape | Sine / Saw / Square / Triangle |
| | Brightness | 0 – 1 |
| Spectral | Manual Level | 0 – 1 |
| | Preset Level | 0 – 1 |
| | Table Level | 0 – 1 |
| | Table Select | Flute / Trumpet C / Violin |
| | Inharmonicity | 0 – 1 |
| | Offset | -1 – 1 |
| | Stretch | 0.5 – 1.5 |
| | Odd/Even Balance | -1 – 1 |
| Amp Env | Attack / Decay / Release | 1 – 2000 ms |
| | Sustain | 0 – 1 |
| Filter | Cutoff | 20 – 20000 Hz |
| | Resonance | 0.1 – 5.0 |
| | Env Amount | 0 – 5000 Hz |
| Filter Env | Attack / Decay / Release | 1 – 2000 ms |
| | Sustain | 0 – 1 |
| Filter LFO | Rate | 0 – 20 Hz |
| | Depth | 0 – 2000 Hz |
| Output | Master Gain | -60 – +24 dB |

## Building

**Prerequisites:** CMake 3.25+, Xcode (macOS) or MSVC (Windows) or GCC/Clang (Linux). JUCE is fetched automatically.

```sh
# 1. Configure (only needed once)
./configure.sh

# 2. Build
./build.sh additive_synth VST3 Release
```

`build.sh` arguments: `<target> <format> <config>`
- **target:** `additive_synth`
- **format:** `VST3` · `AU` · `Standalone` (default: Standalone)
- **config:** `Release` · `Debug` (default: Debug)

Built plugins are copied automatically to:
- macOS VST3: `~/Library/Audio/Plug-Ins/VST3/`
- macOS AU: `~/Library/Audio/Components/`

## Debugging in VSCode

Open the project in VSCode with the [CodeLLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb) extension installed. Press `F5` to build the VST3 in Debug mode and launch REAPER with the debugger attached.

## Project Structure

```
projects/
  AdditiveSynthesizer/   # JUCE plugin (Processor + Editor)
  DSP/
    AddSynth/            # Core additive engine (voices, partial bank, wavetable)
    EnvelopeGenerator    # ADSR
    StateVariableFilter  # SVF
mrta_utils/              # Shared framework (parameters, GUI components)
cmake/                   # add_plugin() helper
```
