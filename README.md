# VOCALCHAX

VOCALCHAX is an original Windows VST3 vocal-transformation plugin built with JUCE. It is a clean-room project: it does not contain Dreamtonics Vocoflex code, assets, voice models, or any activation/licensing bypass.

## What is implemented

- 64-bit Windows VST3 target
- Real-time pitch shifting
- Formant/timbre colour control
- Brightness, body, air, aggression, smoothness, dynamics and breath controls
- Input/output gain and dry/wet mix
- XY morph pad that drives multiple timbral dimensions
- MIDI waypoints on notes 36-39
- MIDI CC20 = morph X, CC21 = morph Y
- DAW parameter automation and state/preset serialization
- Original resizable dark UI
- Optional ONNX Runtime waveform-to-waveform inference backend
- GitHub Actions workflow that builds the Windows VST3 and uploads it as an artifact

The normal build always has the built-in DSP engine, so the plugin remains functional without an AI model.

## Build on GitHub

Open **Actions -> Build Windows VST3 -> Run workflow**, or push to `main`.

When the run succeeds, download the artifact named:

`VOCALCHAX-Windows-VST3`

The artifact contains the `VOCALCHAX.vst3` bundle.

## Local Windows build

Requirements:

- Visual Studio 2022 with Desktop development with C++
- CMake 3.22+
- Git

```powershell
git clone https://github.com/amsiraj1-debug/VOCALCHAX.git
cd VOCALCHAX
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target VOCALCHAX_VST3 --parallel
```

Output:

`build/VOCALCHAX_artefacts/Release/VST3/VOCALCHAX.vst3`

## Optional ONNX Runtime backend

The default CI build has ONNX disabled to keep the VST3 self-contained and easy to build.

To compile ONNX support, download/extract an ONNX Runtime C/C++ package and configure:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DVOCALCHAX_ENABLE_ONNX=ON `
  -DVOCALCHAX_ONNXRUNTIME_ROOT="C:/SDK/onnxruntime"
```

See `models/README.md` for the current waveform model contract. Only use voice models and recordings that you have the right and consent to use.

## Current model contract

The optional adapter takes the first model input and output. It sends a float32 tensor shaped `[1, 1, samples]` and expects float32 waveform audio back. This is intentionally generic so a consent-based model can be swapped in without changing the plugin's UI/DSP architecture.

## Project status

This is an early functional build rather than a clone of any commercial plugin. The built-in pitch/formant processing is DSP-based; higher-fidelity neural voice conversion requires a compatible ONNX model and typically benefits from block buffering, resampling, feature extraction and model-specific conditioning.
