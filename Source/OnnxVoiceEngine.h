#pragma once

#include <JuceHeader.h>
#include <memory>
#include <string>

#if VOCALCHAX_ENABLE_ONNX
#include <onnxruntime_cxx_api.h>
#endif

class OnnxVoiceEngine
{
public:
    OnnxVoiceEngine();
    ~OnnxVoiceEngine();

    bool loadModel (const juce::File& file);
    void unload();
    bool isReady() const noexcept;
    bool process (juce::AudioBuffer<float>& buffer);

    juce::String getLastError() const;
    juce::File getLoadedModel() const;

private:
#if VOCALCHAX_ENABLE_ONNX
    Ort::Env environment;
    Ort::SessionOptions sessionOptions;
    std::unique_ptr<Ort::Session> session;
    std::string inputName;
    std::string outputName;
#endif

    juce::File loadedModel;
    juce::String lastError;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OnnxVoiceEngine)
};
