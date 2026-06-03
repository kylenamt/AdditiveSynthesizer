#include "AdditiveSynthesizerEditor.h"
#include "AdditiveSynthesizerProcessor.h"
#include "WaveformDisplay.h"

#include <algorithm>
#include <array>
#include <vector>

static constexpr int COLUMN_WIDTH { 200 };
static constexpr int COLUMN_GAP { 0 };
static constexpr int COLUMN_COUNT { 3 };
static constexpr int WIDTH { COLUMN_WIDTH * COLUMN_COUNT + COLUMN_GAP * (COLUMN_COUNT - 1) };
static constexpr int PARAM_HEIGHT { 70 };
static constexpr int SECTION_GAP { 12 };
static constexpr int PARTIAL_KNOB_ROWS { 2 };
static constexpr int PARTIAL_KNOB_COLS { 5 };
static constexpr int PARTIAL_SECTION_HEIGHT { 2 * 90 };
static constexpr int PARTIAL_LABEL_HEIGHT { 10 };
static constexpr int PARTIAL_KNOB_PADDING { 4 };
static constexpr int PARTIAL_KNOB_COUNT { PARTIAL_KNOB_ROWS * PARTIAL_KNOB_COLS };

class PartialKnobArray final : public juce::Component
{
public:
    explicit PartialKnobArray(mrta::ParameterManager& parameterManager) :
        apvts(parameterManager.getAPVTS())
    {
        attachments.reserve(PARTIAL_KNOB_COUNT);
        for (int i = 0; i < PARTIAL_KNOB_COUNT; ++i)
        {
            auto& knob = knobs[static_cast<size_t>(i)];
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 16);
            addAndMakeVisible(knob);

            auto& label = labels[static_cast<size_t>(i)];
            label.setText("P" + juce::String(i + 1), juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(label);

            attachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, Param::partialAmpId(i), knob));
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int cellWidth = bounds.getWidth() / PARTIAL_KNOB_COLS;
        const int cellHeight = bounds.getHeight() / PARTIAL_KNOB_ROWS;

        for (int i = 0; i < PARTIAL_KNOB_COUNT; ++i)
        {
            const int row = i / PARTIAL_KNOB_COLS;
            const int col = i % PARTIAL_KNOB_COLS;
            auto cell = juce::Rectangle<int>(bounds.getX() + col * cellWidth,
                                              bounds.getY() + row * cellHeight,
                                              cellWidth,
                                              cellHeight)
                            .reduced(PARTIAL_KNOB_PADDING);

            labels[static_cast<size_t>(i)].setBounds(cell.removeFromTop(PARTIAL_LABEL_HEIGHT));
            knobs[static_cast<size_t>(i)].setBounds(cell);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::array<juce::Slider, PARTIAL_KNOB_COUNT> knobs {};
    std::array<juce::Label, PARTIAL_KNOB_COUNT> labels {};
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
};

static juce::StringArray makeMainParams()
{
    return { Param::ID::NumPartials,
             Param::ID::TableSelect,
             Param::ID::Shape,
             Param::ID::ManualLevel,
             Param::ID::PresetLevel,
             Param::ID::TableLevel,
             Param::ID::MasterGain };
}

static juce::StringArray makeSpectrumParams()
{
    return { Param::ID::Inharmonicity,
             Param::ID::Offset,
             Param::ID::Stretch,
             Param::ID::OddEvenBalance,
             Param::ID::Brightness };
}

static juce::StringArray makeEnvParams()
{
    return { Param::ID::Attack,
             Param::ID::Decay,
             Param::ID::Sustain,
             Param::ID::Release };
}

static juce::StringArray makeFilterEnvParams()
{
    return { Param::ID::FilterAttack,
             Param::ID::FilterDecay,
             Param::ID::FilterSustain,
             Param::ID::FilterRelease };
}

static juce::StringArray makeFilterLfoParams()
{
    return { Param::ID::FilterCutoff,
             Param::ID::FilterReso,
             Param::ID::FilterEnvAmount,
             Param::ID::FilterLfoRate,
             Param::ID::FilterLfoDepth };
}

AdditiveSynthesizerEditor::AdditiveSynthesizerEditor(AdditiveSynthesizerProcessor& p) :
    juce::AudioProcessorEditor(p),
    processor { p },
    mainEditor(processor.getParameterManager(), PARAM_HEIGHT, makeMainParams()),
    spectrumEditor(processor.getParameterManager(), PARAM_HEIGHT, makeSpectrumParams()),
    envEditor(processor.getParameterManager(), PARAM_HEIGHT, makeEnvParams()),
    filterEnvEditor(processor.getParameterManager(), PARAM_HEIGHT, makeFilterEnvParams()),
    filterLfoEditor(processor.getParameterManager(), PARAM_HEIGHT, makeFilterLfoParams())
{
    addAndMakeVisible(mainEditor);
    addAndMakeVisible(spectrumEditor);
    addAndMakeVisible(envEditor);
    addAndMakeVisible(filterEnvEditor);
    addAndMakeVisible(filterLfoEditor);

    // waveform display
    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(frequencyDisplay);
    addAndMakeVisible(viewToggle);
    viewToggle.onClick = [this] { viewToggleChanged(); };

    partialKnobs = std::make_unique<PartialKnobArray>(processor.getParameterManager());
    addAndMakeVisible(*partialKnobs);

    const auto topRows = std::max({ makeMainParams().size(), makeSpectrumParams().size(), makeEnvParams().size(), makeFilterEnvParams().size(), makeFilterLfoParams().size() });
    const auto height = static_cast<int>(topRows) * PARAM_HEIGHT + SECTION_GAP + PARTIAL_SECTION_HEIGHT;
    // five columns: main | spectrum | env | filter env | filter+lfo
    setSize(COLUMN_WIDTH * 5 + COLUMN_GAP * 4, height);

    // initialize displays from current processor state and start timer to poll updates
    waveformDisplay.updateCoefficients(processor.getBlendedBuffer().data(), processor.getRatios().data(), processor.getNumPartials());
    frequencyDisplay.updateCoefficients(processor.getBlendedBuffer().data(), processor.getRatios().data(), processor.getNumPartials());
    startTimerHz(60);
    // set initial visibility: waveform shown by default, frequency hidden
    frequencyDisplay.setVisible(viewToggle.getToggleState());
    waveformDisplay.setVisible(!viewToggle.getToggleState());
}

void AdditiveSynthesizerEditor::timerCallback()
{
    // Poll processor coefficients and update waveform if changed
    const auto& amps = processor.getBlendedBuffer();
    const auto& ratios = processor.getRatios();
    waveformDisplay.updateCoefficients(amps.data(), ratios.data(), processor.getNumPartials());
    frequencyDisplay.updateCoefficients(amps.data(), ratios.data(), processor.getNumPartials());
}

void AdditiveSynthesizerEditor::viewToggleChanged()
{
    const bool showFreq = viewToggle.getToggleState();
    frequencyDisplay.setVisible(showFreq);
    waveformDisplay.setVisible(!showFreq);
    repaint();
}

AdditiveSynthesizerEditor::~AdditiveSynthesizerEditor() = default;

void AdditiveSynthesizerEditor::paint(juce::Graphics&)
{
}

void AdditiveSynthesizerEditor::resized()
{
    auto bounds = getLocalBounds();
    const int topRows = static_cast<int>(std::max({ makeMainParams().size(), makeSpectrumParams().size(), makeEnvParams().size(), makeFilterEnvParams().size(), makeFilterLfoParams().size() }));
    const int topHeight = topRows * PARAM_HEIGHT;

    auto top = bounds.removeFromTop(topHeight);
    bounds.removeFromTop(SECTION_GAP);
    if (partialKnobs)
    {
    auto partialArea = bounds.removeFromTop(PARTIAL_SECTION_HEIGHT);
    // split partial area: left for knobs, right for waveform + toggle
    const int wfWidth = std::max(220, partialArea.getWidth() / 3);
    auto wfArea = partialArea.removeFromRight(wfWidth);
    // Reserve top of wfArea for toggle
    auto toggleArea = wfArea.removeFromTop(24);
    viewToggle.setBounds(toggleArea.reduced(4));
    partialKnobs->setBounds(partialArea);
    // place displays in remaining wfArea
    waveformDisplay.setBounds(wfArea);
    frequencyDisplay.setBounds(wfArea);
    }

    // layout five columns: main | spectrum | env | filter env | filter+lfo
    const int columns = 5;
    const int totalGap = COLUMN_GAP * (columns - 1);
    const int colWidth = (top.getWidth() - totalGap) / columns;

    auto col = top.removeFromLeft(colWidth);
    mainEditor.setBounds(col);

    top.removeFromLeft(COLUMN_GAP);
    col = top.removeFromLeft(colWidth);
    spectrumEditor.setBounds(col);

    top.removeFromLeft(COLUMN_GAP);
    col = top.removeFromLeft(colWidth);
    envEditor.setBounds(col);

    top.removeFromLeft(COLUMN_GAP);
    col = top.removeFromLeft(colWidth);
    filterEnvEditor.setBounds(col);

    top.removeFromLeft(COLUMN_GAP);
    filterLfoEditor.setBounds(top);
}
