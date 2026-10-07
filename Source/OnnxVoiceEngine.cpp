#include "OnnxVoiceEngine.h"

#include <array>
#include <vector>

OnnxVoiceEngine::OnnxVoiceEngine()
#if VOCALCHAX_ENABLE_ONNX
    : environment (ORT_LOGGING_LEVEL_WARNING, "VOCALCHAX")
#endif
{
#if VOCALCHAX_ENABLE_ONNX
    sessionOptions.SetGraphOptimizationLevel (GraphOptimizationLevel::ORT_ENABLE_ALL);
    sessionOptions.SetIntraOpNumThreads (1);
#endif
}

OnnxVoiceEngine::~OnnxVoiceEngine() = default;

bool OnnxVoiceEngine::loadModel (const juce::File& file)
{
    unload();

#if VOCALCHAX_ENABLE_ONNX
    if (! file.existsAsFile())
    {
        lastError = "Model file does not exist.";
        return false;
    }

    try
    {
       #if JUCE_WINDOWS
        const std::wstring path (file.getFullPathName().toWideCharPointer());
        session = std::make_unique<Ort::Session> (environment, path.c_str(), sessionOptions);
       #else
        const std::string path (file.getFullPathName().toStdString());
        session = std::make_unique<Ort::Session> (environment, path.c_str(), sessionOptions);
       #endif

        if (session->GetInputCount() < 1 || session->GetOutputCount() < 1)
            throw std::runtime_error ("Model must have at least one input and one output.");

        Ort::AllocatorWithDefaultOptions allocator;
        auto input = session->GetInputNameAllocated (0, allocator);
        auto output = session->GetOutputNameAllocated (0, allocator);
        inputName = input.get();
        outputName = output.get();

        loadedModel = file;
        lastError.clear();
        return true;
    }
    catch (const std::exception& e)
    {
        session.reset();
        lastError = e.what();
        return false;
    }
#else
    juce::ignoreUnused (file);
    lastError = "This build was compiled without ONNX Runtime. Enable VOCALCHAX_ENABLE_ONNX in CMake.";
    return false;
#endif
}

void OnnxVoiceEngine::unload()
{
#if VOCALCHAX_ENABLE_ONNX
    session.reset();
    inputName.clear();
    outputName.clear();
#endif
    loadedModel = {};
}

bool OnnxVoiceEngine::isReady() const noexcept
{
#if VOCALCHAX_ENABLE_ONNX
    return session != nullptr;
#else
    return false;
#endif
}

bool OnnxVoiceEngine::process (juce::AudioBuffer<float>& buffer)
{
#if VOCALCHAX_ENABLE_ONNX
    if (session == nullptr || buffer.getNumSamples() <= 0 || buffer.getNumChannels() <= 0)
        return false;

    try
    {
        const auto samples = buffer.getNumSamples();
        const auto channels = buffer.getNumChannels();

        std::vector<float> mono (static_cast<size_t> (samples), 0.0f);
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto* source = buffer.getReadPointer (channel);
            for (int i = 0; i < samples; ++i)
                mono[static_cast<size_t> (i)] += source[i] / static_cast<float> (channels);
        }

        const std::array<int64_t, 3> shape { 1, 1, static_cast<int64_t> (samples) };
        auto memoryInfo = Ort::MemoryInfo::CreateCpu (OrtArenaAllocator, OrtMemTypeDefault);
        auto inputTensor = Ort::Value::CreateTensor<float> (memoryInfo,
                                                             mono.data(),
                                                             mono.size(),
                                                             shape.data(),
                                                             shape.size());

        const char* inputNames[] { inputName.c_str() };
        const char* outputNames[] { outputName.c_str() };

        auto outputs = session->Run (Ort::RunOptions { nullptr },
                                     inputNames,
                                     &inputTensor,
                                     1,
                                     outputNames,
                                     1);

        if (outputs.empty() || ! outputs.front().IsTensor())
            return false;

        auto info = outputs.front().GetTensorTypeAndShapeInfo();
        const auto outputCount = info.GetElementCount();
        const auto* output = outputs.front().GetTensorData<float>();

        if (output == nullptr || outputCount == 0)
            return false;

        const auto copyCount = juce::jmin (static_cast<size_t> (samples), outputCount);
        for (int channel = 0; channel < channels; ++channel)
        {
            auto* destination = buffer.getWritePointer (channel);
            for (size_t i = 0; i < copyCount; ++i)
                destination[static_cast<int> (i)] = output[i];
        }

        return true;
    }
    catch (const std::exception& e)
    {
        lastError = e.what();
        return false;
    }
#else
    juce::ignoreUnused (buffer);
    return false;
#endif
}

juce::String OnnxVoiceEngine::getLastError() const
{
    return lastError;
}

juce::File OnnxVoiceEngine::getLoadedModel() const
{
    return loadedModel;
}
