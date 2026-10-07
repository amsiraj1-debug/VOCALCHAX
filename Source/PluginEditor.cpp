#include "PluginEditor.h"

namespace
{
const auto background = juce::Colour::fromRGB (12, 14, 20);
const auto panel = juce::Colour::fromRGB (23, 27, 38);
const auto panelBorder = juce::Colour::fromRGB (48, 57, 76);
const auto accent = juce::Colour::fromRGB (115, 100, 255);
const auto cyan = juce::Colour::fromRGB (73, 213, 255);
const auto text = juce::Colour::fromRGB (238, 241, 248);
const auto muted = juce::Colour::fromRGB (145, 153, 174);
}

MorphPad::MorphPad (juce::AudioProcessorValueTreeState& state)
{
    xParameter = state.getParameter ("morphX");
    yParameter = state.getParameter ("morphY");
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    startTimerHz (30);
}

MorphPad::~MorphPad()
{
    stopTimer();
}

void MorphPad::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (8.0f);

    juce::ColourGradient gradient (juce::Colour::fromRGB (40, 32, 87),
                                   bounds.getTopLeft(),
                                   juce::Colour::fromRGB (15, 69, 87),
                                   bounds.getBottomRight(),
                                   false);
    gradient.addColour (0.50, juce::Colour::fromRGB (42, 34, 61));

    g.setGradientFill (gradient);
    g.fillRoundedRectangle (bounds, 18.0f);

    g.setColour (panelBorder);
    g.drawRoundedRectangle (bounds, 18.0f, 1.5f);

    auto inner = bounds.reduced (18.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (int i = 1; i < 4; ++i)
    {
        const auto x = inner.getX() + inner.getWidth() * static_cast<float> (i) / 4.0f;
        const auto y = inner.getY() + inner.getHeight() * static_cast<float> (i) / 4.0f;
        g.drawVerticalLine (juce::roundToInt (x), inner.getY(), inner.getBottom());
        g.drawHorizontalLine (juce::roundToInt (y), inner.getX(), inner.getRight());
    }

    const float x = xParameter != nullptr ? xParameter->getValue() : 0.5f;
    const float y = yParameter != nullptr ? yParameter->getValue() : 0.5f;

    const auto marker = juce::Point<float> (
        juce::jmap (x, inner.getX(), inner.getRight()),
        juce::jmap (1.0f - y, inner.getY(), inner.getBottom()));

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (juce::Rectangle<float> (28.0f, 28.0f).withCentre (marker).translated (2.0f, 3.0f));

    g.setColour (cyan);
    g.fillEllipse (juce::Rectangle<float> (24.0f, 24.0f).withCentre (marker));

    g.setColour (juce::Colours::white);
    g.drawEllipse (juce::Rectangle<float> (24.0f, 24.0f).withCentre (marker), 1.5f);

    g.setColour (text);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("MORPH SPACE", inner.withHeight (26.0f), juce::Justification::topLeft);

    g.setColour (muted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("X: character   Y: air / body", inner.removeFromBottom (22.0f), juce::Justification::bottomLeft);
}

void MorphPad::mouseDown (const juce::MouseEvent& event)
{
    if (xParameter != nullptr)
        xParameter->beginChangeGesture();
    if (yParameter != nullptr)
        yParameter->beginChangeGesture();

    updateFromPosition (event.position);
}

void MorphPad::mouseDrag (const juce::MouseEvent& event)
{
    updateFromPosition (event.position);
}

void MorphPad::mouseUp (const juce::MouseEvent& event)
{
    updateFromPosition (event.position);

    if (xParameter != nullptr)
        xParameter->endChangeGesture();
    if (yParameter != nullptr)
        yParameter->endChangeGesture();
}

void MorphPad::updateFromPosition (juce::Point<float> point)
{
    const auto inner = getLocalBounds().toFloat().reduced (26.0f);

    const float x = juce::jlimit (0.0f, 1.0f, (point.x - inner.getX()) / juce::jmax (1.0f, inner.getWidth()));
    const float y = 1.0f - juce::jlimit (0.0f, 1.0f, (point.y - inner.getY()) / juce::jmax (1.0f, inner.getHeight()));

    if (xParameter != nullptr)
        xParameter->setValueNotifyingHost (x);

    if (yParameter != nullptr)
        yParameter->setValueNotifyingHost (y);

    repaint();
}

void MorphPad::timerCallback()
{
    repaint();
}

VocalChaxAudioProcessorEditor::VocalChaxAudioProcessorEditor (VocalChaxAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      morphPad (p.apvts)
{
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (820, 540, 1500, 950);
    setSize (1060, 680);

    titleLabel.setText ("VOCALCHAX", juce::dontSendNotification);
    titleLabel.setColour (juce::Label::textColourId, text);
    titleLabel.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("REAL-TIME VOCAL MORPH ENGINE", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, muted);
    subtitleLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (subtitleLabel);

    addAndMakeVisible (morphPad);

    setupKnob (pitchSlider, "pitch", "PITCH");
    setupKnob (formantSlider, "formant", "FORMANT");
    setupKnob (brightnessSlider, "brightness", "BRIGHT");
    setupKnob (bodySlider, "body", "BODY");
    setupKnob (airSlider, "air", "AIR");
    setupKnob (aggressionSlider, "aggression", "AGGRESSION");
    setupKnob (smoothnessSlider, "smoothness", "SMOOTH");
    setupKnob (dynamicsSlider, "dynamics", "DYNAMICS");
    setupKnob (breathSlider, "breath", "BREATH");
    setupKnob (inputSlider, "inputGain", "INPUT");
    setupKnob (outputSlider, "outputGain", "OUTPUT");
    setupKnob (mixSlider, "mix", "MIX");

    engineBox.addItemList ({ "DSP", "ONNX" }, 1);
    engineBox.setColour (juce::ComboBox::backgroundColourId, panel);
    engineBox.setColour (juce::ComboBox::textColourId, text);
    engineBox.setColour (juce::ComboBox::outlineColourId, panelBorder);
    addAndMakeVisible (engineBox);
    engineAttachment = std::make_unique<ComboAttachment> (processor.apvts, "engine", engineBox);

    loadModelButton.setColour (juce::TextButton::buttonColourId, accent);
    loadModelButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (loadModelButton);

    loadModelButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Choose an ONNX waveform model",
            juce::File {},
            "*.onnx");

        juce::Component::SafePointer<VocalChaxAudioProcessorEditor> safeThis (this);

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectFiles,
                                  [safeThis] (const juce::FileChooser& chooser)
                                  {
                                      if (safeThis == nullptr)
                                          return;

                                      const auto file = chooser.getResult();
                                      if (file.existsAsFile())
                                          safeThis->processor.loadOnnxModel (file);

                                      safeThis->timerCallback();
                                  });
    };

    statusLabel.setColour (juce::Label::textColourId, muted);
    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    startTimerHz (4);
    timerCallback();
}

VocalChaxAudioProcessorEditor::~VocalChaxAudioProcessorEditor()
{
    stopTimer();
}

void VocalChaxAudioProcessorEditor::setupKnob (juce::Slider& slider,
                                                const juce::String& parameterID,
                                                const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, panelBorder);
    slider.setColour (juce::Slider::thumbColourId, cyan);
    slider.setColour (juce::Slider::textBoxTextColourId, text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    auto label = std::make_unique<juce::Label>();
    label->setText (labelText, juce::dontSendNotification);
    label->setColour (juce::Label::textColourId, muted);
    label->setFont (juce::FontOptions (11.0f, juce::Font::bold));
    label->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (*label);

    knobs.push_back (&slider);
    knobLabels.push_back (std::move (label));
    sliderAttachments.push_back (
        std::make_unique<SliderAttachment> (processor.apvts, parameterID, slider));
}

void VocalChaxAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient glow (accent.withAlpha (0.13f),
                               bounds.getTopRight() - juce::Point<float> (130.0f, -20.0f),
                               juce::Colours::transparentBlack,
                               bounds.getCentre(),
                               true);
    g.setGradientFill (glow);
    g.fillRect (bounds);

    g.setColour (panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (18.0f).withTrimmedTop (72.0f).withTrimmedBottom (72.0f), 16.0f);

    g.setColour (panelBorder);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (18.0f).withTrimmedTop (72.0f).withTrimmedBottom (72.0f), 16.0f, 1.0f);

    g.setColour (muted.withAlpha (0.85f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("MIDI waypoints: notes 36-39  |  CC20 = X  |  CC21 = Y",
                24, getHeight() - 34, getWidth() - 48, 18,
                juce::Justification::centredRight);
}

void VocalChaxAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (24);

    auto header = bounds.removeFromTop (62);
    titleLabel.setBounds (header.removeFromTop (38));
    subtitleLabel.setBounds (header);

    auto footer = bounds.removeFromBottom (72);
    bounds.removeFromBottom (8);

    const int leftWidth = juce::jlimit (280, 410, static_cast<int> (bounds.getWidth() * 0.36f));
    auto left = bounds.removeFromLeft (leftWidth);
    bounds.removeFromLeft (12);

    morphPad.setBounds (left.reduced (8));

    auto grid = bounds.reduced (8);
    constexpr int columns = 4;
    constexpr int rows = 3;

    const int cellWidth = grid.getWidth() / columns;
    const int cellHeight = grid.getHeight() / rows;

    for (size_t index = 0; index < knobs.size(); ++index)
    {
        const int column = static_cast<int> (index) % columns;
        const int row = static_cast<int> (index) / columns;

        auto cell = juce::Rectangle<int> (
            grid.getX() + column * cellWidth,
            grid.getY() + row * cellHeight,
            cellWidth,
            cellHeight).reduced (5);

        knobLabels[index]->setBounds (cell.removeFromTop (22));
        knobs[index]->setBounds (cell);
    }

    auto footerInner = footer.reduced (8);
    engineBox.setBounds (footerInner.removeFromLeft (130).reduced (0, 12));
    footerInner.removeFromLeft (10);
    loadModelButton.setBounds (footerInner.removeFromLeft (175).reduced (0, 10));
    footerInner.removeFromLeft (14);
    statusLabel.setBounds (footerInner.reduced (0, 8));
}

void VocalChaxAudioProcessorEditor::timerCallback()
{
    statusLabel.setText (processor.getOnnxStatus(), juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId,
                           processor.isOnnxReady() ? cyan : muted);
}
