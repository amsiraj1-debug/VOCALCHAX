#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ParameterIDs
{
constexpr auto inputGain = "inputGain";
constexpr auto outputGain = "outputGain";
constexpr auto mix = "mix";
constexpr auto pitch = "pitch";
constexpr auto formant = "formant";
constexpr auto brightness = "brightness";
constexpr auto body = "body";
constexpr auto air = "air";
constexpr auto aggression = "aggression";
constexpr auto smoothness = "smoothness";
constexpr auto dynamics = "dynamics";
constexpr auto breath = "breath";
constexpr auto morphX = "morphX";
constexpr auto morphY = "morphY";
constexpr auto engine = "engine";
}

VocalChaxAudioProcessor::VocalChaxAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOCALCHAX_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalChaxAudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::inputGain, 1 }, "Input Gain",
        Range { -24.0f, 24.0f, 0.01f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::outputGain, 1 }, "Output Gain",
        Range { -24.0f, 24.0f, 0.01f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::mix, 1 }, "Mix",
        Range { 0.0f, 1.0f, 0.001f }, 1.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::pitch, 1 }, "Pitch",
        Range { -12.0f, 12.0f, 0.01f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::formant, 1 }, "Formant",
        Range { -1.0f, 1.0f, 0.001f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::brightness, 1 }, "Brightness",
        Range { -1.0f, 1.0f, 0.001f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::body, 1 }, "Body",
        Range { -1.0f, 1.0f, 0.001f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::air, 1 }, "Air",
        Range { -1.0f, 1.0f, 0.001f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::aggression, 1 }, "Aggression",
        Range { 0.0f, 1.0f, 0.001f }, 0.10f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::smoothness, 1 }, "Smoothness",
        Range { 0.0f, 1.0f, 0.001f }, 0.10f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::dynamics, 1 }, "Dynamics",
        Range { 0.0f, 1.0f, 0.001f }, 0.20f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::breath, 1 }, "Breath",
        Range { 0.0f, 1.0f, 0.001f }, 0.0f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::morphX, 1 }, "Morph X",
        Range { 0.0f, 1.0f, 0.001f }, 0.5f));

    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParameterIDs::morphY, 1 }, "Morph Y",
        Range { 0.0f, 1.0f, 0.001f }, 0.5f));

    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParameterIDs::engine, 1 }, "Engine",
        juce::StringArray { "DSP", "ONNX" }, 0));

    return { parameters.begin(), parameters.end() };
}

void VocalChaxAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    vocalShaper.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock, false, false, true);
}

void VocalChaxAudioProcessor::releaseResources()
{
    vocalShaper.reset();
    dryBuffer.setSize (0, 0);
}

bool VocalChaxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;

    return input == output;
}

VocalShaper::Parameters VocalChaxAudioProcessor::readShaperParameters() const
{
    VocalShaper::Parameters p;

    p.pitchSemitones = apvts.getRawParameterValue (ParameterIDs::pitch)->load();
    p.formant = apvts.getRawParameterValue (ParameterIDs::formant)->load();
    p.brightness = apvts.getRawParameterValue (ParameterIDs::brightness)->load();
    p.body = apvts.getRawParameterValue (ParameterIDs::body)->load();
    p.air = apvts.getRawParameterValue (ParameterIDs::air)->load();
    p.aggression = apvts.getRawParameterValue (ParameterIDs::aggression)->load();
    p.smoothness = apvts.getRawParameterValue (ParameterIDs::smoothness)->load();
    p.dynamics = apvts.getRawParameterValue (ParameterIDs::dynamics)->load();
    p.breath = apvts.getRawParameterValue (ParameterIDs::breath)->load();
    p.morphX = apvts.getRawParameterValue (ParameterIDs::morphX)->load();
    p.morphY = apvts.getRawParameterValue (ParameterIDs::morphY)->load();

    return p;
}

void VocalChaxAudioProcessor::applyMidiWaypoints (const juce::MidiBuffer& midi)
{
    auto* xParameter = apvts.getParameter (ParameterIDs::morphX);
    auto* yParameter = apvts.getParameter (ParameterIDs::morphY);

    if (xParameter == nullptr || yParameter == nullptr)
        return;

    const auto setXY = [xParameter, yParameter] (float x, float y)
    {
        xParameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, x));
        yParameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, y));
    };

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            switch (message.getNoteNumber())
            {
                case 36: setXY (0.15f, 0.15f); break;
                case 37: setXY (0.85f, 0.15f); break;
                case 38: setXY (0.15f, 0.85f); break;
                case 39: setXY (0.85f, 0.85f); break;
                default: break;
            }
        }
        else if (message.isController())
        {
            const float value = static_cast<float> (message.getControllerValue()) / 127.0f;

            if (message.getControllerNumber() == 20)
                xParameter->setValueNotifyingHost (value);
            else if (message.getControllerNumber() == 21)
                yParameter->setValueNotifyingHost (value);
        }
    }
}

void VocalChaxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    applyMidiWaypoints (midi);

    if (dryBuffer.getNumChannels() != buffer.getNumChannels()
        || dryBuffer.getNumSamples() < buffer.getNumSamples())
    {
        dryBuffer.setSize (buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, 0, buffer.getNumSamples());

    const float inputGain = juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (ParameterIDs::inputGain)->load());

    buffer.applyGain (inputGain);

    const int engineChoice = static_cast<int> (
        apvts.getRawParameterValue (ParameterIDs::engine)->load());

    if (engineChoice == 1 && onnxEngine.isReady())
        onnxEngine.process (buffer);

    vocalShaper.process (buffer, readShaperParameters());

    const float outputGain = juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (ParameterIDs::outputGain)->load());

    buffer.applyGain (outputGain);

    const float mix = juce::jlimit (
        0.0f, 1.0f, apvts.getRawParameterValue (ParameterIDs::mix)->load());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* wet = buffer.getWritePointer (channel);
        const auto* dry = dryBuffer.getReadPointer (channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            wet[i] = dry[i] + (wet[i] - dry[i]) * mix;
    }
}

void VocalChaxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();

    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalChaxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        const auto state = juce::ValueTree::fromXml (*xml);

        if (state.isValid())
            apvts.replaceState (state);
    }
}

bool VocalChaxAudioProcessor::loadOnnxModel (const juce::File& file)
{
    return onnxEngine.loadModel (file);
}

bool VocalChaxAudioProcessor::isOnnxReady() const noexcept
{
    return onnxEngine.isReady();
}

juce::String VocalChaxAudioProcessor::getOnnxStatus() const
{
    if (onnxEngine.isReady())
        return "ONNX: " + onnxEngine.getLoadedModel().getFileName();

    const auto error = onnxEngine.getLastError();
    return error.isNotEmpty() ? error : "ONNX: no model loaded";
}

juce::AudioProcessorEditor* VocalChaxAudioProcessor::createEditor()
{
    return new VocalChaxAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalChaxAudioProcessor();
}
