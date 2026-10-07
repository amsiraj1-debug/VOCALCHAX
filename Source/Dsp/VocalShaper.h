#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include <vector>

class VocalShaper
{
public:
    struct Parameters
    {
        float pitchSemitones = 0.0f;
        float formant = 0.0f;
        float brightness = 0.0f;
        float body = 0.0f;
        float air = 0.0f;
        float aggression = 0.0f;
        float smoothness = 0.0f;
        float dynamics = 0.0f;
        float breath = 0.0f;
        float morphX = 0.5f;
        float morphY = 0.5f;
    };

    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Parameters& parameters);

private:
    struct PitchState
    {
        void prepare (double newSampleRate);
        void reset();
        float processSample (float input, float semitones);

        float readDelayed (float delaySamples) const;

        std::vector<float> ring;
        int writeIndex = 0;
        double sampleRate = 48000.0;
        float phase = 0.0f;
    };

    struct ChannelState
    {
        PitchState pitch;
        float lowBody = 0.0f;
        float lowPresence = 0.0f;
        float lowAir = 0.0f;
        float smooth = 0.0f;
        float envelope = 0.0f;
        float previousNoise = 0.0f;
    };

    static float onePoleCoefficient (float cutoff, double sampleRate);
    float nextNoise();

    double sampleRate = 48000.0;
    std::vector<ChannelState> states;
    std::uint32_t noiseState = 0x6d2b79f5u;
};
