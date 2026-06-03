#pragma once

#include <BaseProcessor.h>
#include <memory>

#include "GenericParameterEditor.h"
#include "WaveformDisplay.h"
#include "AdditiveSynthesizerProcessor.h"

class PartialKnobArray;

class AdditiveSynthesizerEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    AdditiveSynthesizerEditor(AdditiveSynthesizerProcessor&);
    ~AdditiveSynthesizerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AdditiveSynthesizerProcessor& processor;

    mrta::GenericParameterEditor mainEditor;
    mrta::GenericParameterEditor spectrumEditor;
    mrta::GenericParameterEditor envEditor;
    mrta::GenericParameterEditor filterEnvEditor;
    mrta::GenericParameterEditor filterLfoEditor;
    std::unique_ptr<PartialKnobArray> partialKnobs;
    WaveformDisplay waveformDisplay;
    FrequencyDisplay frequencyDisplay;
    juce::ToggleButton viewToggle { "Spectral" };

    void viewToggleChanged();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdditiveSynthesizerEditor)
};
