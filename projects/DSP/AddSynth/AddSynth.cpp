#include "AddSynth.h"

#include <algorithm>
#include <cmath>

namespace DSP
{
float convertMidiNoteToFreq(int midiNote)
{
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

AdditiveVoice::AdditiveVoice()
{
    envGen.setAnalogStyle(false);
    filterGen.setAnalogStyle(false);
}

AdditiveVoice::~AdditiveVoice() = default;

void AdditiveVoice::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;
    // prepare filter and envelopes
    filter.prepare(sampleRate);

    envGen.prepare(newSampleRate);
    filterGen.prepare(newSampleRate);
    partialBank.prepare(sampleRate);
    partialBank.randomizePhases(random);
    filterCutoffRamp.prepare(sampleRate, true, filterCutoffHz);
    filterResonanceRamp.prepare(sampleRate, true, filterResonance);
    filterEnvAmountRamp.prepare(sampleRate, true, filterEnvAmount);
    filterLfoRateRamp.prepare(sampleRate, true, filterLfoRate);
    filterLfoDepthRamp.prepare(sampleRate, true, filterLfoDepth);
    voiceGainRamp.prepare(sampleRate, true, voiceGain);
    noteVelocityRamp.prepare(sampleRate, true, noteVelocity);
}

// update all params at once
void AdditiveVoice::syncParams(const SynthParams& params)
{
    envGen.setAttackTime(params.attackMs);
    envGen.setDecayTime(params.decayMs);
    envGen.setReleaseTime(params.releaseMs);
    envGen.setSustainLevel(params.sustain);
    // filter envelope
    filterGen.setAttackTime(params.filterAttackMs);
    filterGen.setDecayTime(params.filterDecayMs);
    filterGen.setReleaseTime(params.filterReleaseMs);
    filterGen.setSustainLevel(params.filterSustain);
    filterCutoffHz = params.filterCutoffHz;
    filterResonance = params.filterResonance;
    filterEnvAmount = params.filterEnvAmount;
    filterLfoRate = params.filterLfoRate;
    filterLfoDepth = params.filterLfoDepth;
    partialBank.setInharmonicity(params.inharmonicity);
    partialBank.setOffset(params.offset);
    partialBank.setStretch(params.stretch);
    partialBank.setOddEvenBalance(params.oddEvenBalance);
    voiceGain = params.masterGain;

    filterCutoffRamp.setTarget(filterCutoffHz);
    filterResonanceRamp.setTarget(filterResonance);
    filterEnvAmountRamp.setTarget(filterEnvAmount);
    filterLfoRateRamp.setTarget(filterLfoRate);
    filterLfoDepthRamp.setTarget(filterLfoDepth);
    voiceGainRamp.setTarget(voiceGain);

    partialBank.setNumPartials(params.numPartials);
    partialBank.setRatios(params.ratios);
    partialBank.setCoefficients(params.amplitudes);
}

bool AdditiveVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<AdditiveSound*>(sound) != nullptr;
}

void AdditiveVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    noteVelocity = velocity;
    noteVelocityRamp.setTarget(noteVelocity);
    partialBank.setBaseFrequency(convertMidiNoteToFreq(midiNoteNumber));
    envGen.start();
    filterGen.start();
}

void AdditiveVoice::stopNote(float, bool allowTailOff)
{
    envGen.end();
    filterGen.end();
    if (!allowTailOff)
    {
        clearCurrentNote();
    }
}

void AdditiveVoice::pitchWheelMoved(int)
{
}

void AdditiveVoice::controllerMoved(int, int)
{
}

void AdditiveVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    if (numSamples <= 0)
        return;

    const double currentSampleRate = getSampleRate();
    if (currentSampleRate != sampleRate)
        prepare(currentSampleRate, numSamples);

    const int numChannels = outputBuffer.getNumChannels();

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        float envValue = {0.f};
        envGen.process(&envValue, 1);

        float filterEnvValue = {0.f};
        filterGen.process(&filterEnvValue, 1);

        const float cutoffBase = filterCutoffRamp.getNext();
        const float envAmount = filterEnvAmountRamp.getNext();
        const float lfoRateValue = filterLfoRateRamp.getNext();
        const float lfoDepthValue = filterLfoDepthRamp.getNext();
        const float resonanceValue = filterResonanceRamp.getNext();
        const float gainValue = voiceGainRamp.getNext();
        const float velocityValue = noteVelocityRamp.getNext();

        // update filter LFO phase delta according to ramp
        if (sampleRate > 0.0)
            filterLfoPhaseDelta = 2*M_PI * lfoRateValue / static_cast<float>(sampleRate);
        else
            filterLfoPhaseDelta = 0.0f;

        // update filter LFO phase and wrap around
        float lfoValue = 0.0f;
        if (lfoDepthValue != 0.0f)
        {
            lfoValue = std::sin(filterLfoPhase);
            filterLfoPhase += filterLfoPhaseDelta;
            if (filterLfoPhase >= 2*M_PI)
                filterLfoPhase -= 2*M_PI;
        }

        // calculate cutoff frequency
        float cutoff = cutoffBase + envAmount * filterEnvValue + lfoDepthValue * lfoValue;
        cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);

        // generate dry sample with partial bank
        float sampleValue = partialBank.processSample();
        sampleValue *= envValue * velocityValue * gainValue; // scaling

        float lpfOut = 0.0f, bpfOut = 0.0f, hpfOut = 0.0f;

        filter.process(&lpfOut, &bpfOut, &hpfOut, &sampleValue, &cutoff, &resonanceValue, 1);
        sampleValue = lpfOut;// use low-pass path

        // write to the output buffer
        const int bufferIndex = startSample + sampleIndex;
        for (int ch = 0; ch < numChannels; ++ch)
            outputBuffer.addSample(ch, bufferIndex, sampleValue);
    }
    
    // clear note once note-off
    if (envGen.isOff() && filterGen.isOff())
        clearCurrentNote();
}

void AdditiveSynthesiser::prepare(double newSampleRate, int samplesPerBlock)
{
    setCurrentPlaybackSampleRate(newSampleRate);
    for (int i = 0; i < getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<AdditiveVoice*>(getVoice(i)))
            voice->prepare(newSampleRate, samplesPerBlock);
}

void AdditiveSynthesiser::updateParams(const SynthParams& params)
{
    SynthParams localParams = params;
    if (getNumVoices() > 0)
        localParams.masterGain = params.masterGain / static_cast<float>(getNumVoices());

    for (int i = 0; i < getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<AdditiveVoice*>(getVoice(i)))
            voice->syncParams(localParams);
}
}
