#include "AdditiveSynthesizerProcessor.h"
#include "AdditiveSynthesizerEditor.h"
#include "AddSynth/HarmonicTable_Fl.h"
#include "AddSynth/HarmonicTable_TpC.h"
#include "AddSynth/HarmonicTable_Vn.h"

#include <algorithm>
#include <cmath>
#include <vector>


static const std::vector<mrta::ParameterInfo> paramVector = []()
{
    std::vector<mrta::ParameterInfo> params;
    params.reserve(24 + DSP::kManualPartials);

    params.emplace_back(Param::ID::NumPartials, Param::Name::NumPartials, "",
                        32.0f, Param::Range::NumPartialsMin, Param::Range::NumPartialsMax,
                        Param::Range::NumPartialsInc, Param::Range::NumPartialsSkw);
    params.emplace_back(Param::ID::TableSelect, Param::Name::TableSelect, Param::Range::TableOptions, 0);
    params.emplace_back(Param::ID::Shape, Param::Name::Shape, Param::Range::ShapeOptions, 0);
    params.emplace_back(Param::ID::Brightness, Param::Name::Brightness, "",
                        1.0f, 0.5f, 1.0f, 0.01f, 1.0f);
    params.emplace_back(Param::ID::ManualLevel, Param::Name::ManualLevel, "",
                        1.0f, Param::Range::LevelMin, Param::Range::LevelMax, Param::Range::LevelInc, Param::Range::LevelSkw);
    params.emplace_back(Param::ID::PresetLevel, Param::Name::PresetLevel, "",
                        1.0f, Param::Range::LevelMin, Param::Range::LevelMax, Param::Range::LevelInc, Param::Range::LevelSkw);
    params.emplace_back(Param::ID::TableLevel, Param::Name::TableLevel, "",
                        1.0f, Param::Range::LevelMin, Param::Range::LevelMax, Param::Range::LevelInc, Param::Range::LevelSkw);
    params.emplace_back(Param::ID::Inharmonicity, Param::Name::Inharmonicity, "",
                        0.0f, Param::Range::InharmonicityMin, Param::Range::InharmonicityMax,
                        Param::Range::InharmonicityInc, Param::Range::InharmonicitySkw);
    params.emplace_back(Param::ID::Offset, Param::Name::Offset, "",
                        0.0f, Param::Range::OffsetMin, Param::Range::OffsetMax,
                        Param::Range::OffsetInc, Param::Range::OffsetSkw);
    params.emplace_back(Param::ID::Stretch, Param::Name::Stretch, "",
                        1.0f, Param::Range::StretchMin, Param::Range::StretchMax,
                        Param::Range::StretchInc, Param::Range::StretchSkw);
    params.emplace_back(Param::ID::OddEvenBalance, Param::Name::OddEvenBalance, "",
                        0.0f, Param::Range::OddEvenBalanceMin, Param::Range::OddEvenBalanceMax,
                        Param::Range::OddEvenBalanceInc, Param::Range::OddEvenBalanceSkw);
    params.emplace_back(Param::ID::Attack, Param::Name::Attack, Param::Unit::Ms,
                        10.0f, Param::Range::TimeMin, Param::Range::TimeMax,
                        Param::Range::TimeInc, Param::Range::TimeSkw);
    params.emplace_back(Param::ID::Decay, Param::Name::Decay, Param::Unit::Ms,
                        80.0f, Param::Range::TimeMin, Param::Range::TimeMax,
                        Param::Range::TimeInc, Param::Range::TimeSkw);
    params.emplace_back(Param::ID::Sustain, Param::Name::Sustain, "",
                        0.7f, Param::Range::SustainMin, Param::Range::SustainMax,
                        Param::Range::SustainInc, Param::Range::SustainSkw);
    params.emplace_back(Param::ID::Release, Param::Name::Release, Param::Unit::Ms,
                        120.0f, Param::Range::TimeMin, Param::Range::TimeMax,
                        Param::Range::TimeInc, Param::Range::TimeSkw);
    params.emplace_back(Param::ID::FilterCutoff, Param::Name::FilterCutoff, Param::Unit::Hz,
                        2000.0f, Param::Range::FilterCutoffMin, Param::Range::FilterCutoffMax,
                        Param::Range::FilterCutoffInc, Param::Range::FilterCutoffSkw);
    params.emplace_back(Param::ID::FilterReso, Param::Name::FilterReso, "",
                        0.7f, Param::Range::FilterResoMin, Param::Range::FilterResoMax,
                        Param::Range::FilterResoInc, Param::Range::FilterResoSkw);
    params.emplace_back(Param::ID::FilterEnvAmount, Param::Name::FilterEnvAmount, "",
                        0.0f, Param::Range::FilterEnvMin, Param::Range::FilterEnvMax,
                        Param::Range::FilterEnvInc, Param::Range::FilterEnvSkw);
    params.emplace_back(Param::ID::FilterAttack, Param::Name::FilterAttack, Param::Unit::Ms,
                        10.0f, Param::Range::TimeMin, Param::Range::TimeMax,
                        Param::Range::TimeInc, Param::Range::TimeSkw);
    params.emplace_back(Param::ID::FilterDecay, Param::Name::FilterDecay, Param::Unit::Ms,
                        80.0f, Param::Range::TimeMin, Param::Range::TimeMax,
                        Param::Range::TimeInc, Param::Range::TimeSkw);
    params.emplace_back(Param::ID::FilterSustain, Param::Name::FilterSustain, "",
                        0.7f, Param::Range::SustainMin, Param::Range::SustainMax,
                        Param::Range::SustainInc, Param::Range::SustainSkw);
    params.emplace_back(Param::ID::FilterRelease, Param::Name::FilterRelease, Param::Unit::Ms,
                        120.0f, Param::Range::TimeMin, Param::Range::TimeMax,
                        Param::Range::TimeInc, Param::Range::TimeSkw);
    params.emplace_back(Param::ID::FilterLfoRate, Param::Name::FilterLfoRate, Param::Unit::Hz,
                        0.0f, Param::Range::LfoRateMin, Param::Range::LfoRateMax,
                        Param::Range::LfoRateInc, Param::Range::LfoRateSkw);
    params.emplace_back(Param::ID::FilterLfoDepth, Param::Name::FilterLfoDepth, Param::Unit::Hz,
                        0.0f, Param::Range::LfoDepthMin, Param::Range::LfoDepthMax,
                        Param::Range::LfoDepthInc, Param::Range::LfoDepthSkw);
    params.emplace_back(Param::ID::MasterGain, Param::Name::MasterGain, Param::Unit::Db,
                        0.0f, Param::Range::MasterGainMin, Param::Range::MasterGainMax,
                        Param::Range::MasterGainInc, Param::Range::MasterGainSkw);
    for (int i = 0; i < DSP::kManualPartials; ++i)
    {
        const float def = (i == 0) ? 1.0f : 0.0f;
        params.emplace_back(Param::partialAmpId(i), Param::partialAmpName(i), "",
                            def, Param::Range::PartialAmpMin, Param::Range::PartialAmpMax,
                            Param::Range::PartialAmpInc, Param::Range::PartialAmpSkw);
    }

    return params;
}();

AdditiveSynthesizerProcessor::AdditiveSynthesizerProcessor() :
    mrta::BaseProcessor(paramVector)
{
    synth.addSound(new DSP::AdditiveSound());
    for (int i = 0; i < NumVoices; ++i)
        synth.addVoice(new DSP::AdditiveVoice());

    // Match Synth: don't steal still-sounding voices (avoids mid-note resets/clicks).
    synth.setNoteStealingEnabled(false);

    for (int i = 0; i < DSP::kMaxPartials; ++i)
    {
        const float harmonic = static_cast<float>(i + 1);
        harmonicRatios[static_cast<size_t>(i)] = harmonic;
        ratios[static_cast<size_t>(i)] = harmonic;
    }

    spectralCoeffs.setManualAmp(0, 1.0f);

    registerParameterCallback(Param::ID::NumPartials,
        [this] (float value, bool)
        {
            const int clamped = std::clamp(static_cast<int>(std::round(value)), 1, DSP::kMaxPartials);
            numPartials = clamped;
        });

    registerParameterCallback(Param::ID::TableSelect,
        [this] (float value, bool)
        {
            // map dropdown index to TableSource via small lookup table
            static constexpr DSP::SpectralCoefficients::TableSource tableMap[] = {
                DSP::SpectralCoefficients::TableSource::None,
                DSP::SpectralCoefficients::TableSource::Flute,
                DSP::SpectralCoefficients::TableSource::TrumpetC,
                DSP::SpectralCoefficients::TableSource::Violin
            };
            const int clamped = std::clamp(static_cast<int>(std::round(value)), 0, static_cast<int>(std::size(tableMap) - 1));
            spectralCoeffs.setTableSource(tableMap[clamped]);
        });

    registerParameterCallback(Param::ID::Shape,
        [this] (float value, bool)
        {
            spectralCoeffs.setPresetShape(static_cast<DSP::SpectralCoefficients::PresetShape>(std::round(value)));
        });

    registerParameterCallback(Param::ID::Brightness,
        [this] (float value, bool)
        {
            // UI: lower slider = visually "down". Map so slider up = brighter.
            const float brightnessMin = 0.5f;
            const float brightnessMax = 1.0f;
            const float mapped = brightnessMin + brightnessMax - value; // invert within [min,max]
            spectralCoeffs.setPresetBrightness(mapped);
        });

    registerParameterCallback(Param::ID::ManualLevel,[this] (float value, bool){spectralCoeffs.setManualLevel(value);});
    registerParameterCallback(Param::ID::PresetLevel,[this] (float value, bool){spectralCoeffs.setPresetLevel(value);});
    registerParameterCallback(Param::ID::TableLevel,[this] (float value, bool){spectralCoeffs.setTableLevel(value);});
    registerParameterCallback(Param::ID::Inharmonicity,[this] (float value, bool){inharmonicity = std::clamp(value, Param::Range::InharmonicityMin, Param::Range::InharmonicityMax);});
    registerParameterCallback(Param::ID::Offset,[this] (float value, bool){offset = std::clamp(value, Param::Range::OffsetMin, Param::Range::OffsetMax);});
    registerParameterCallback(Param::ID::Stretch,[this] (float value, bool){stretch = std::clamp(value, Param::Range::StretchMin, Param::Range::StretchMax);});
    registerParameterCallback(Param::ID::OddEvenBalance,[this] (float value, bool){oddEvenBalance = std::clamp(value, Param::Range::OddEvenBalanceMin, Param::Range::OddEvenBalanceMax);});
    
    registerParameterCallback(Param::ID::Attack,[this] (float value, bool){attackMs = value;});
    registerParameterCallback(Param::ID::Decay,[this] (float value, bool){decayMs = value;});
    registerParameterCallback(Param::ID::Sustain,[this] (float value, bool){sustain = value;});
    registerParameterCallback(Param::ID::Release,[this] (float value, bool){releaseMs = value;});
    registerParameterCallback(Param::ID::FilterCutoff,[this] (float value, bool){filterCutoffHz = value;});
    registerParameterCallback(Param::ID::FilterReso,[this] (float value, bool){filterReso = value;});
    registerParameterCallback(Param::ID::FilterEnvAmount,[this] (float value, bool){filterEnvAmount = value;});
    registerParameterCallback(Param::ID::FilterAttack,[this] (float value, bool){filterAttackMs = value;});
    registerParameterCallback(Param::ID::FilterDecay,[this] (float value, bool){filterDecayMs = value;});
    registerParameterCallback(Param::ID::FilterSustain,[this] (float value, bool){filterSustain = value;});
    registerParameterCallback(Param::ID::FilterRelease,[this] (float value, bool){filterReleaseMs = value;});
    registerParameterCallback(Param::ID::FilterLfoRate,[this] (float value, bool){filterLfoRate = value;});
    registerParameterCallback(Param::ID::FilterLfoDepth,[this] (float value, bool){filterLfoDepth = value;});
    registerParameterCallback(Param::ID::MasterGain,[this] (float value, bool){masterGainDb = value;});

    for (int i = 0; i < DSP::kManualPartials; ++i)
    {
        registerParameterCallback(Param::partialAmpId(i),
            [this, i] (float value, bool)
            {
                spectralCoeffs.setManualAmp(i, value);
            });
    }
}

AdditiveSynthesizerProcessor::~AdditiveSynthesizerProcessor() = default;

void AdditiveSynthesizerProcessor::prepare(double sampleRate, int samplesPerBlock)
{
    synth.prepare(sampleRate, samplesPerBlock);
    updateSynthParams();
    synth.updateParams(synthParams);
}

void AdditiveSynthesizerProcessor::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
            lastMidiNote = message.getNoteNumber();
    }
    updateSynthParams();
    synth.updateParams(synthParams);
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* AdditiveSynthesizerProcessor::createEditor()
{
    return new AdditiveSynthesizerEditor(*this);
}

void AdditiveSynthesizerProcessor::updateSynthParams()
{
    // calculate stretched frequency for display purposes
    const float baseFrequency = DSP::convertMidiNoteToFreq(lastMidiNote);
    const float effectiveB = (baseFrequency > 0.0f) ? (inharmonicity / baseFrequency) : 0.0f;

    // mirror PartialBank::updateFrequencies so the displays match what is heard
    for (int i = 0; i < DSP::kMaxPartials; ++i)
    {
        float harmonic = 1.0f + (harmonicRatios[static_cast<size_t>(i)] - 1.0f) * stretch;
        harmonic += offset;
        harmonic += oddEvenBalance * (((i + 1) % 2 != 0) ? 1.0f : -1.0f);
        ratios[static_cast<size_t>(i)] = std::max(0.0f, harmonic * std::sqrt(1.0f + effectiveB * harmonic * harmonic));
    }

    // Compute blended coefficients via SpectralCoefficients
    // Velocity amplitude is handled per-voice via noteVelocityRamp, not here.
    spectralCoeffs.computeCoefficients(blendedBuffer, numPartials, 60, 100);
    synthParams.amplitudes = blendedBuffer;
    synthParams.ratios = harmonicRatios;
    synthParams.numPartials = numPartials;
    synthParams.attackMs = attackMs;
    synthParams.decayMs = decayMs;
    synthParams.sustain = sustain;
    synthParams.releaseMs = releaseMs;
    synthParams.filterCutoffHz = filterCutoffHz;
    synthParams.filterResonance = filterReso;
    synthParams.filterEnvAmount = filterEnvAmount;
    synthParams.filterAttackMs = filterAttackMs;
    synthParams.filterDecayMs = filterDecayMs;
    synthParams.filterSustain = filterSustain;
    synthParams.filterReleaseMs = filterReleaseMs;
    synthParams.filterLfoRate = filterLfoRate;
    synthParams.filterLfoDepth = filterLfoDepth;
    synthParams.inharmonicity = inharmonicity;
    synthParams.offset = offset;
    synthParams.stretch = stretch;
    synthParams.oddEvenBalance = oddEvenBalance;
    synthParams.masterGain = std::pow(10.0f, 0.05f * masterGainDb);
}

CREATE_PLUGIN(AdditiveSynthesizerProcessor)
