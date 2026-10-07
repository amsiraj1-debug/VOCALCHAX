#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

class MorphPad final : public juce::Component,
                       private juce::Timer
{
public:
    explicit MorphPad (juce::AudioProcessorValueTreeState& state);
    ~MorphPad() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void updateFromPosition (juce::Point<float> point);

    juce::RangedAudioParameter* xParameter = nullptr;
    juce::RangedAudioParameter* yParameter = nullptr;
};

class VocalChaxAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit VocalChaxAudioProcessorEditor (VocalChaxAudioProcessor&);
    ~VocalChaxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void setupKnob (juce::Slider& slider, const juce::String& parameterID, const juce::String& labelText);

    VocalChaxAudioProcessor& processor;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    MorphPad morphPad;

    juce::Slider pitchSlider;
    juce::Slider formantSlider;
    juce::Slider brightnessSlider;
    juce::Slider bodySlider;
    juce::Slider airSlider;
    juce::Slider aggressionSlider;
    juce::Slider smoothnessSlider;
    juce::Slider dynamicsSlider;
    juce::Slider breathSlider;
    juce::Slider inputSlider;
    juce::Slider outputSlider;
    juce::Slider mixSlider;

    std::vector<juce::Slider*> knobs;
    std::vector<std::unique_ptr<juce::Label>> knobLabels;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;

    juce::ComboBox engineBox;
    std::unique_ptr<ComboAttachment> engineAttachment;

    juce::TextButton loadModelButton { "LOAD ONNX MODEL" };
    juce::Label statusLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChaxAudioProcessorEditor)
};
