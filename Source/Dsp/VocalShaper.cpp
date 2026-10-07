#include "VocalShaper.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;

float clampBipolar (float value)
{
    return juce::jlimit (-1.0f, 1.0f, value);
}

float clampUnit (float value)
{
    return juce::jlimit (0.0f, 1.0f, value);
}
}

void VocalShaper::PitchState::prepare (double newSampleRate)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);
    const auto size = juce::jmax (2048, static_cast<int> (sampleRate * 0.25));
    ring.assign (static_cast<size_t> (size), 0.0f);
    reset();
}

void VocalShaper::PitchState::reset()
{
    std::fill (ring.begin(), ring.end(), 0.0f);
    writeIndex = 0;
    phase = 0.0f;
}

float VocalShaper::PitchState::readDelayed (float delaySamples) const
{
    if (ring.empty())
        return 0.0f;

    const auto size = static_cast<int> (ring.size());
    float position = static_cast<float> (writeIndex) - delaySamples;

    while (position < 0.0f)
        position += static_cast<float> (size);

    while (position >= static_cast<float> (size))
        position -= static_cast<float> (size);

    const auto indexA = static_cast<int> (position);
    const auto indexB = (indexA + 1) % size;
    const auto fraction = position - static_cast<float> (indexA);

    return juce::jmap (fraction, ring[static_cast<size_t> (indexA)], ring[static_cast<size_t> (indexB)]);
}

float VocalShaper::PitchState::processSample (float input, float semitones)
{
    if (ring.empty())
        return input;

    ring[static_cast<size_t> (writeIndex)] = input;

    if (std::abs (semitones) < 0.01f)
    {
        writeIndex = (writeIndex + 1) % static_cast<int> (ring.size());
        return input;
    }

    const float ratio = std::pow (2.0f, semitones / 12.0f);
    const float minimumDelay = 64.0f;
    const float range = juce::jlimit (192.0f,
                                      static_cast<float> (ring.size()) - minimumDelay - 4.0f,
                                      static_cast<float> (sampleRate * 0.045));

    const float phaseIncrement = std::abs (1.0f - ratio) / range;
    const float phaseA = phase;
    const float phaseB = std::fmod (phase + 0.5f, 1.0f);

    const auto delayForPhase = [ratio, minimumDelay, range] (float p)
    {
        return minimumDelay + range * (ratio >= 1.0f ? (1.0f - p) : p);
    };

    const auto window = [] (float p)
    {
        return 0.5f - 0.5f * std::cos (twoPi * p);
    };

    const float weightA = window (phaseA);
    const float weightB = window (phaseB);
    const float sampleA = readDelayed (delayForPhase (phaseA));
    const float sampleB = readDelayed (delayForPhase (phaseB));
    const float output = (sampleA * weightA + sampleB * weightB) / juce::jmax (0.001f, weightA + weightB);

    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;

    writeIndex = (writeIndex + 1) % static_cast<int> (ring.size());
    return output;
}

void VocalShaper::prepare (double newSampleRate, int, int channels)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);
    states.resize (static_cast<size_t> (juce::jmax (1, channels)));

    for (auto& state : states)
        state.pitch.prepare (sampleRate);

    reset();
}

void VocalShaper::reset()
{
    for (auto& state : states)
    {
        state.pitch.reset();
        state.lowBody = 0.0f;
        state.lowPresence = 0.0f;
        state.lowAir = 0.0f;
        state.smooth = 0.0f;
        state.envelope = 0.0f;
        state.previousNoise = 0.0f;
    }
}

float VocalShaper::onePoleCoefficient (float cutoff, double sr)
{
    const float safeCutoff = juce::jlimit (20.0f, static_cast<float> (sr * 0.45), cutoff);
    return 1.0f - std::exp (-twoPi * safeCutoff / static_cast<float> (sr));
}

float VocalShaper::nextNoise()
{
    noiseState = noiseState * 1664525u + 1013904223u;
    const auto normalized = static_cast<float> ((noiseState >> 8) & 0x00ffffffu) / 8388607.5f - 1.0f;
    return normalized;
}

void VocalShaper::process (juce::AudioBuffer<float>& buffer, const Parameters& inputParameters)
{
    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
        return;

    Parameters p = inputParameters;

    // The morph pad intentionally changes several timbral dimensions at once.
    p.brightness = clampBipolar (p.brightness + (p.morphX - 0.5f) * 0.55f);
    p.body       = clampBipolar (p.body       + (0.5f - p.morphY) * 0.45f);
    p.air        = clampBipolar (p.air        + (p.morphY - 0.5f) * 0.50f);
    p.aggression = clampUnit    (p.aggression + p.morphX * p.morphY * 0.25f);

    const float bodyAlpha = onePoleCoefficient (180.0f, sampleRate);
    const float presenceCentre = juce::jmap (p.formant, -1.0f, 1.0f, 650.0f, 2300.0f);
    const float presenceAlpha = onePoleCoefficient (presenceCentre, sampleRate);
    const float airAlpha = onePoleCoefficient (7000.0f, sampleRate);
    const float smoothCutoff = juce::jmap (clampUnit (p.smoothness), 18000.0f, 4500.0f);
    const float smoothAlpha = onePoleCoefficient (smoothCutoff, sampleRate);

    const float compressorThreshold = juce::jmap (clampUnit (p.dynamics), 0.95f, 0.22f);
    const float compressionStrength = juce::jmap (clampUnit (p.dynamics), 0.0f, 0.82f);
    const float attack = onePoleCoefficient (30.0f, sampleRate);
    const float release = onePoleCoefficient (3.0f, sampleRate);

    const float drive = 1.0f + clampUnit (p.aggression) * 8.0f;
    const float driveNorm = std::tanh (drive);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);
        auto& state = states[static_cast<size_t> (channel % static_cast<int> (states.size()))];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = state.pitch.processSample (samples[i], p.pitchSemitones);

            state.lowBody += bodyAlpha * (x - state.lowBody);
            state.lowPresence += presenceAlpha * (x - state.lowPresence);
            state.lowAir += airAlpha * (x - state.lowAir);

            const float bodyBand = state.lowBody;
            const float presenceBand = state.lowPresence - state.lowBody;
            const float airBand = x - state.lowAir;

            x += bodyBand * p.body * 0.75f;
            x += presenceBand * p.formant * 0.90f;
            x += (x - state.lowPresence) * p.brightness * 0.50f;
            x += airBand * p.air * 0.42f;

            if (p.aggression > 0.001f)
                x = std::tanh (x * drive) / juce::jmax (0.001f, driveNorm);

            state.smooth += smoothAlpha * (x - state.smooth);
            x = juce::jmap (clampUnit (p.smoothness), x, state.smooth);

            const float detector = std::abs (x);
            const float envCoeff = detector > state.envelope ? attack : release;
            state.envelope += envCoeff * (detector - state.envelope);

            if (state.envelope > compressorThreshold)
            {
                const float targetGain = compressorThreshold / juce::jmax (compressorThreshold, state.envelope);
                x *= juce::jmap (compressionStrength, 1.0f, targetGain);
            }

            if (p.breath > 0.001f)
            {
                const float noise = nextNoise();
                const float highNoise = noise - state.previousNoise * 0.85f;
                state.previousNoise = noise;
                const float gate = juce::jlimit (0.0f, 1.0f, detector * 8.0f);
                x += highNoise * gate * p.breath * 0.018f;
            }

            samples[i] = juce::jlimit (-1.5f, 1.5f, x);
        }
    }
}
