#pragma once

#include <BaseProcessor.h>
#include <memory>

#include "GenericParameterEditor.h"
#include "WaveformDisplay.h"
#include "AdditiveSynthesizerProcessor.h"
#include "CustomLookAndFeel.h"

class PartialKnobArray;
class SectionHeader;

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
    CustomLookAndFeel customLookAndFeel;

    mrta::GenericParameterEditor mainEditor;
    mrta::GenericParameterEditor spectrumEditor;
    mrta::GenericParameterEditor envEditor;
    mrta::GenericParameterEditor filterEnvEditor;
    mrta::GenericParameterEditor filterLfoEditor;
    std::unique_ptr<PartialKnobArray> partialKnobs;
    WaveformDisplay waveformDisplay;
    FrequencyDisplay frequencyDisplay;
    juce::ToggleButton viewToggle { "Spectral" };

    juce::ConcertinaPanel concertina;
    std::unique_ptr<SectionHeader> headerMain;
    std::unique_ptr<SectionHeader> headerSpectrum;
    std::unique_ptr<SectionHeader> headerEnv;
    std::unique_ptr<SectionHeader> headerFilterEnv;
    std::unique_ptr<SectionHeader> headerFilterLfo;

    void addSection(mrta::GenericParameterEditor& editor,
                    std::unique_ptr<SectionHeader>& header,
                    const juce::String& title, int paramCount, bool startExpanded);
    void togglePanel(juce::Component* panel, SectionHeader* header, int contentHeight);

    void viewToggleChanged();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdditiveSynthesizerEditor)
};
