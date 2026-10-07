# Optional ONNX voice models

VOCALCHAX works without a model by using its built-in real-time DSP voice shaper.

The optional ONNX backend is intended for models you have the legal right and consent to use. Models are deliberately not committed to this repository.

## Minimal model contract

The current adapter expects a float32 waveform input and waveform output.

- First input: float32 tensor shaped `[1, 1, samples]` or otherwise accepting that shape.
- First output: float32 waveform tensor with at least `samples` values.
- Input/output sample rate should match the host sample rate unless your model handles resampling internally.

The adapter discovers the first input and output names dynamically.

## Enable ONNX Runtime

Configure CMake with:

```powershell
cmake -S . -B build -A x64 `
  -DVOCALCHAX_ENABLE_ONNX=ON `
  -DVOCALCHAX_ONNXRUNTIME_ROOT="C:/SDK/onnxruntime"
```

The ONNX Runtime package should contain `include/onnxruntime_cxx_api.h` and `lib/onnxruntime.lib`.
