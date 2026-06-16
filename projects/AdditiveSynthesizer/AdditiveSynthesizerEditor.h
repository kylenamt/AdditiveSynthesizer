#pragma once

#include <BaseProcessor.h>
#include <memory>

#include "GenericParameterEditor.h"
#include "WaveformDisplay.h"
#include "AdditiveSynthesizerProcessor.h"
#include "CustomLookAndFeel.h"

class PartialKnobArray;
class BandEnvPanel;
class SectionHeader;

// TabbedButtonBar reports tab changes via a virtual, not a listener interface.
class CallbackTabBar final : public juce::TabbedButtonBar
{
public:
    CallbackTabBar() : juce::TabbedButtonBar(juce::TabbedButtonBar::TabsAtTop) {}

    std::function<void()> onChange;

    void currentTabChanged(int, const juce::String&) override
    {
        if (onChange) onChange();
    }
};

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
    std::unique_ptr<BandEnvPanel> bandEnvPanel;
    WaveformDisplay waveformDisplay;
    FrequencyDisplay frequencyDisplay;
    FilterShapeDisplay filterDisplay;
    ADSRDisplay adsrDisplay;
    CallbackTabBar viewTabs;

    enum ViewTab { TabWave = 0, TabSpectrum, TabFilter, TabAdsr };

    std::unique_ptr<SectionHeader> headerMain;
    std::unique_ptr<SectionHeader> headerSpectrum;
    std::unique_ptr<SectionHeader> headerEnv;
    std::unique_ptr<SectionHeader> headerBandEnv;
    std::unique_ptr<SectionHeader> headerFilterEnv;
    std::unique_ptr<SectionHeader> headerFilterLfo;

    // Declared last so it is destroyed FIRST — the ConcertinaPanel references the
    // section headers and panel components above without owning them.
    juce::ConcertinaPanel concertina;

    void addSection(mrta::GenericParameterEditor& editor,
                    std::unique_ptr<SectionHeader>& header,
                    const juce::String& title, int paramCount, bool startExpanded);
    void addComponentSection(juce::Component& content,
                             std::unique_ptr<SectionHeader>& header,
                             const juce::String& title, int contentHeight, bool startExpanded);
    void togglePanel(juce::Component* panel, SectionHeader* header, int contentHeight);

    void updateDisplayVisibility();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdditiveSynthesizerEditor)
};
