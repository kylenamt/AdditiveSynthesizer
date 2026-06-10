#pragma once

#include <BaseProcessor.h>

#include "AddSynth/AddSynth.h"
#include "AddSynth/SpectralCoef.h"

namespace Param
{
    namespace ID
    {
        static const juce::String NumPartials { "num_partials" };
        static const juce::String Shape { "shape" };
        static const juce::String Brightness { "Brightness" };
        static const juce::String ManualLevel { "manual_level" };
        static const juce::String PresetLevel { "preset_level" };
        static const juce::String TableLevel { "table_level" };
        static const juce::String TableSelect { "table_select" };
        static const juce::String Inharmonicity { "inharmonicity" };
        static const juce::String Offset { "offset" };
        static const juce::String Stretch { "stretch" };
        static const juce::String OddEvenBalance { "odd_even_balance" };

        static const juce::String Attack { "attack" };
        static const juce::String Decay { "decay" };
        static const juce::String Sustain { "sustain" };
        static const juce::String Release { "release" };

        static const juce::String EnvMode { "env_mode" };

        static const juce::String FilterCutoff { "filter_cutoff" };
        static const juce::String FilterReso { "filter_reso" };
        static const juce::String FilterEnvAmount { "filter_env_amount" };
        static const juce::String FilterAttack { "filter_attack" };
        static const juce::String FilterDecay { "filter_decay" };
        static const juce::String FilterSustain { "filter_sustain" };
        static const juce::String FilterRelease { "filter_release" };

        static const juce::String FilterLfoRate { "filter_lfo_rate" };
        static const juce::String FilterLfoDepth { "filter_lfo_depth" };

        static const juce::String MasterGain { "master_gain" };
    }

    namespace Name
    {
        static const juce::String NumPartials { "Num Partials" };
        static const juce::String Shape { "Shape" };
        static const juce::String Brightness { "Brightness" };
        static const juce::String ManualLevel { "Manual Level" };
        static const juce::String PresetLevel { "Preset Level" };
        static const juce::String TableLevel { "Table Level" };
        static const juce::String TableSelect { "Table Select" };
        static const juce::String Inharmonicity { "Inharmonicity" };
        static const juce::String Offset { "Offset" };
        static const juce::String Stretch { "Stretch" };
        static const juce::String OddEvenBalance { "Odd/Even Balance" };

        static const juce::String Attack { "Attack" };
        static const juce::String Decay { "Decay" };
        static const juce::String Sustain { "Sustain" };
        static const juce::String Release { "Release" };

        static const juce::String EnvMode { "Env Mode" };

        static const juce::String FilterCutoff { "Filter Cutoff" };
        static const juce::String FilterReso { "Filter Resonance" };
        static const juce::String FilterEnvAmount { "Filter Env Amount" };
        static const juce::String FilterAttack { "Filter Attack" };
        static const juce::String FilterDecay { "Filter Decay" };
        static const juce::String FilterSustain { "Filter Sustain" };
        static const juce::String FilterRelease { "Filter Release" };

        static const juce::String FilterLfoRate { "Filter LFO Rate" };
        static const juce::String FilterLfoDepth { "Filter LFO Depth" };

        static const juce::String MasterGain { "Master Gain" };
    }

    namespace Unit
    {
        static const juce::String Ms { "ms" };
        static const juce::String Hz { "Hz" };
        static const juce::String Db { "dB" };
    }

    namespace Range
    {
        static constexpr float PartialAmpMin { 0.0f };
        static constexpr float PartialAmpMax { 1.0f };
        static constexpr float PartialAmpInc { 0.001f };
        static constexpr float PartialAmpSkw { 1.0f };

        static constexpr float NumPartialsMin { 1.0f };
        static constexpr float NumPartialsMax { static_cast<float>(DSP::kMaxPartials) };
        static constexpr float NumPartialsInc { 1.0f };
        static constexpr float NumPartialsSkw { 1.0f };

        static constexpr float TimeMin { 1.0f };
        static constexpr float TimeMax { 2000.0f };
        static constexpr float TimeInc { 1.0f };
        static constexpr float TimeSkw { 0.5f };

        static constexpr float SustainMin { 0.0f };
        static constexpr float SustainMax { 1.0f };
        static constexpr float SustainInc { 0.001f };
        static constexpr float SustainSkw { 1.0f };

        static constexpr float FilterCutoffMin { 20.0f };
        static constexpr float FilterCutoffMax { 20000.0f };
        static constexpr float FilterCutoffInc { 1.0f };
        static constexpr float FilterCutoffSkw { 0.5f };

        static constexpr float FilterResoMin { 0.1f };
        static constexpr float FilterResoMax { 5.0f };
        static constexpr float FilterResoInc { 0.01f };
        static constexpr float FilterResoSkw { 0.5f };

        static constexpr float FilterEnvMin { 0.0f };
        static constexpr float FilterEnvMax { 5000.0f };
        static constexpr float FilterEnvInc { 1.0f };
        static constexpr float FilterEnvSkw { 0.5f };

        static constexpr float LfoRateMin { 0.0f };
        static constexpr float LfoRateMax { 20.0f };
        static constexpr float LfoRateInc { 0.01f };
        static constexpr float LfoRateSkw { 0.5f };

        static constexpr float LfoDepthMin { 0.0f };
        static constexpr float LfoDepthMax { 2000.0f };
        static constexpr float LfoDepthInc { 1.0f };
        static constexpr float LfoDepthSkw { 0.5f };

        static constexpr float MasterGainMin { -60.0f };
        static constexpr float MasterGainMax { 24.0f };
        static constexpr float MasterGainInc { 0.1f };
        static constexpr float MasterGainSkw { 2.8f };

        static constexpr float InharmonicityMin { 0.0f };
        static constexpr float InharmonicityMax { 1.0f };
        static constexpr float InharmonicityInc { 0.001f };
        static constexpr float InharmonicitySkw { 1.0f };

        static constexpr float OffsetMin { -1.0f };
        static constexpr float OffsetMax { 1.0f };
        static constexpr float OffsetInc { 0.001f };
        static constexpr float OffsetSkw { 1.0f };

        static constexpr float StretchMin { 0.5f };
        static constexpr float StretchMax { 1.5f };
        static constexpr float StretchInc { 0.001f };
        static constexpr float StretchSkw { 1.0f };

        static constexpr float OddEvenBalanceMin { -1.0f };
        static constexpr float OddEvenBalanceMax { 1.0f };
        static constexpr float OddEvenBalanceInc { 0.001f };
        static constexpr float OddEvenBalanceSkw { 1.0f };

        static const juce::StringArray CoeffSourceOptions { "Manual", "Preset", "Flute", "Trumpet C", "Violin" };
        static const juce::StringArray ShapeOptions { "Sine", "Saw", "Square", "Triangle" };
        static const juce::StringArray TableOptions { "None", "Flute", "Trumpet C", "Violin" };
        static const juce::StringArray EnvModeOptions { "Global", "Per-Band" };
        static constexpr float LevelMin { 0.0f };
        static constexpr float LevelMax { 1.0f };
        static constexpr float LevelInc { 0.001f };
        static constexpr float LevelSkw { 1.0f };
    }

    inline juce::String partialAmpId(int index)
    {
        return "partial_amp_" + juce::String(index);
    }

    inline juce::String partialAmpName(int index)
    {
        return "Partial Amp " + juce::String(index + 1);
    }

    // Per-band envelope parameters. stage: 0=Attack, 1=Decay, 2=Sustain, 3=Release.
    namespace BandStage { static const juce::StringArray ids { "attack", "decay", "sustain", "release" };
                          static const juce::StringArray names { "Attack", "Decay", "Sustain", "Release" }; }

    inline juce::String bandParamId(int band, int stage)
    {
        return "band" + juce::String(band) + "_" + BandStage::ids[stage];
    }

    inline juce::String bandParamName(int band, int stage)
    {
        return "Band " + juce::String(band + 1) + " " + BandStage::names[stage];
    }

    inline juce::String bandCrossoverId(int index)
    {
        return "band_crossover_" + juce::String(index);
    }

    inline juce::String bandCrossoverName(int index)
    {
        return "Crossover " + juce::String(index + 1);
    }
}

class AdditiveSynthesizerProcessor : public mrta::BaseProcessor
{
public:
    AdditiveSynthesizerProcessor();
    ~AdditiveSynthesizerProcessor() override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;

private:
    void updateSynthParams();

    DSP::AdditiveSynthesiser synth;
    DSP::SpectralCoefficients spectralCoeffs;

    DSP::SynthParams synthParams;
    std::array<float, DSP::kMaxPartials> blendedBuffer {};
    std::array<float, DSP::kMaxPartials> ratios {};
    std::array<float, DSP::kMaxPartials> harmonicRatios {};

    int numPartials { 32 };
    float attackMs { 10.0f };
    float decayMs { 80.0f };
    float sustain { 0.7f };
    float releaseMs { 120.0f };

    bool perBandEnv { false };
    std::array<float, DSP::kNumCrossovers> bandCrossoverHz { 150.0f, 600.0f, 2000.0f, 6000.0f };
    std::array<float, DSP::kNumBands> bandAttackMs  { 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
    std::array<float, DSP::kNumBands> bandDecayMs   { 80.0f, 80.0f, 80.0f, 80.0f, 80.0f };
    std::array<float, DSP::kNumBands> bandSustain   { 0.7f, 0.7f, 0.7f, 0.7f, 0.7f };
    std::array<float, DSP::kNumBands> bandReleaseMs { 120.0f, 120.0f, 120.0f, 120.0f, 120.0f };
    float filterCutoffHz { 2000.0f };
    float filterReso { 0.7f };
    float filterEnvAmount { 0.0f };
    float filterAttackMs { 10.0f };
    float filterDecayMs { 80.0f };
    float filterSustain { 0.7f };
    float filterReleaseMs { 120.0f };
    float filterLfoRate { 0.0f };
    float filterLfoDepth { 0.0f };
    float inharmonicity { 0.0f };
    float offset { 0.0f };
    float stretch { 1.0f };
    float oddEvenBalance { 0.0f };
    float masterGainDb { 0.0f };
    
    int lastMidiNote { 60 };

    static constexpr int NumVoices { 8 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdditiveSynthesizerProcessor)
public:
    // Expose current coefficients for the editor (safe const access)
    const std::array<float, DSP::kMaxPartials>& getBlendedBuffer() const { return blendedBuffer; }
    const std::array<float, DSP::kMaxPartials>& getRatios() const { return ratios; }
    int getNumPartials() const { return numPartials; }
};
