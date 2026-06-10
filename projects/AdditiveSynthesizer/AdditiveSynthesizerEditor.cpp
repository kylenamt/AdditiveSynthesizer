#include "AdditiveSynthesizerEditor.h"
#include "AdditiveSynthesizerProcessor.h"
#include "WaveformDisplay.h"

#include <algorithm>
#include <array>
#include <vector>

static constexpr int PARAM_HEIGHT { 70 };
static constexpr int HEADER_HEIGHT { 28 };
static constexpr int ACCORDION_WIDTH { 380 };
static constexpr int COLUMN_GAP { 12 };
static constexpr int MARGIN { 12 };
static constexpr int TOGGLE_HEIGHT { 26 };
static constexpr int PARTIAL_KNOB_ROWS { 2 };
static constexpr int PARTIAL_KNOB_COLS { 5 };
static constexpr int PARTIAL_SECTION_HEIGHT { 200 };
static constexpr int PARTIAL_LABEL_HEIGHT { 10 };
static constexpr int PARTIAL_KNOB_PADDING { 4 };
static constexpr int PARTIAL_KNOB_COUNT { PARTIAL_KNOB_ROWS * PARTIAL_KNOB_COLS };

// Clickable accordion section header with a disclosure triangle.
class SectionHeader final : public juce::Component
{
public:
    SectionHeader(const juce::String& titleText) : title(titleText) {}

    std::function<void()> onToggle;

    void setExpanded(bool shouldBeExpanded)
    {
        expanded = shouldBeExpanded;
        repaint();
    }

    bool isExpanded() const { return expanded; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colour(CustomLookAndFeel::colSurface));
        g.fillRect(bounds);
        g.setColour(juce::Colour(CustomLookAndFeel::colBorder));
        g.drawRect(bounds, 1.0f);

        // Disclosure triangle on the left
        const float cx = bounds.getX() + 14.0f;
        const float cy = bounds.getCentreY();
        const float s = 4.0f;
        juce::Path tri;
        if (expanded)
            tri.addTriangle(cx - s, cy - s, cx + s, cy - s, cx, cy + s); // ▼
        else
            tri.addTriangle(cx - s, cy - s, cx - s, cy + s, cx + s, cy); // ▶
        g.setColour(juce::Colour(isMouseOver ? CustomLookAndFeel::colAccentHover
                                             : CustomLookAndFeel::colAccent));
        g.fillPath(tri);

        // Title
        auto textBounds = getLocalBounds().withTrimmedLeft(28);
        g.setColour(juce::Colour(isMouseOver ? CustomLookAndFeel::colTextPrimary
                                             : CustomLookAndFeel::colTextSecondary));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(title, textBounds, juce::Justification::centredLeft);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains(e.getPosition()) && onToggle)
            onToggle();
    }

    void mouseEnter(const juce::MouseEvent&) override { isMouseOver = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { isMouseOver = false; repaint(); }

private:
    juce::String title;
    bool expanded { false };
    bool isMouseOver { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionHeader)
};

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
    setLookAndFeel(&customLookAndFeel);

    // Accordion of collapsible parameter sections
    addAndMakeVisible(concertina);
    addSection(mainEditor,      headerMain,      "MAIN",          static_cast<int>(makeMainParams().size()),      true);
    addSection(spectrumEditor,  headerSpectrum,  "SPECTRUM",      static_cast<int>(makeSpectrumParams().size()),  false);
    addSection(envEditor,       headerEnv,       "ENVELOPE",      static_cast<int>(makeEnvParams().size()),       false);
    addSection(filterEnvEditor, headerFilterEnv, "FILTER ENV",    static_cast<int>(makeFilterEnvParams().size()), false);
    addSection(filterLfoEditor, headerFilterLfo, "FILTER + LFO",  static_cast<int>(makeFilterLfoParams().size()), false);

    // waveform display
    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(frequencyDisplay);
    addAndMakeVisible(viewToggle);
    viewToggle.onClick = [this] { viewToggleChanged(); };

    partialKnobs = std::make_unique<PartialKnobArray>(processor.getParameterManager());
    addAndMakeVisible(*partialKnobs);

    // Window tall enough to fully show the largest section plus the other collapsed headers.
    const int largestSection = static_cast<int>(std::max({ makeMainParams().size(), makeSpectrumParams().size(), makeEnvParams().size(), makeFilterEnvParams().size(), makeFilterLfoParams().size() }));
    const int accordionHeight = largestSection * PARAM_HEIGHT + 5 * HEADER_HEIGHT;
    const int rightHeight = TOGGLE_HEIGHT + PARTIAL_SECTION_HEIGHT + 200; // display + knobs
    const int contentHeight = std::max(accordionHeight, rightHeight);
    setSize(MARGIN * 2 + ACCORDION_WIDTH + COLUMN_GAP + 430, contentHeight + MARGIN * 2);

    // initialize displays from current processor state and start timer to poll updates
    waveformDisplay.updateCoefficients(processor.getBlendedBuffer().data(), processor.getRatios().data(), processor.getNumPartials());
    frequencyDisplay.updateCoefficients(processor.getBlendedBuffer().data(), processor.getRatios().data(), processor.getNumPartials());
    startTimerHz(60);
    // set initial visibility: waveform shown by default, frequency hidden
    frequencyDisplay.setVisible(viewToggle.getToggleState());
    waveformDisplay.setVisible(!viewToggle.getToggleState());
}

void AdditiveSynthesizerEditor::addSection(mrta::GenericParameterEditor& editor,
                                           std::unique_ptr<SectionHeader>& header,
                                           const juce::String& title, int paramCount,
                                           bool startExpanded)
{
    const int contentHeight = paramCount * PARAM_HEIGHT;

    header = std::make_unique<SectionHeader>(title);

    concertina.addPanel(-1, &editor, false);
    concertina.setPanelHeaderSize(&editor, HEADER_HEIGHT);
    concertina.setCustomPanelHeader(&editor, header.get(), false);
    concertina.setMaximumPanelSize(&editor, contentHeight);

    auto* headerPtr = header.get();
    header->onToggle = [this, editor = &editor, headerPtr, contentHeight]
    {
        togglePanel(editor, headerPtr, contentHeight);
    };

    header->setExpanded(startExpanded);
    concertina.setPanelSize(&editor, startExpanded ? contentHeight : 0, false);
}

void AdditiveSynthesizerEditor::togglePanel(juce::Component* panel, SectionHeader* header, int contentHeight)
{
    const bool nowExpanded = !header->isExpanded();
    header->setExpanded(nowExpanded);
    concertina.setPanelSize(panel, nowExpanded ? contentHeight : 0, true);
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

AdditiveSynthesizerEditor::~AdditiveSynthesizerEditor()
{
    setLookAndFeel(nullptr);
}

void AdditiveSynthesizerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(CustomLookAndFeel::colBackground));
}

void AdditiveSynthesizerEditor::resized()
{
    auto bounds = getLocalBounds().reduced(MARGIN);

    // Left: accordion of collapsible parameter sections
    auto left = bounds.removeFromLeft(ACCORDION_WIDTH);
    concertina.setBounds(left);

    bounds.removeFromLeft(COLUMN_GAP);

    // Right: always-visible display + partial knobs
    auto right = bounds;

    // Partial knobs docked at the bottom
    if (partialKnobs)
        partialKnobs->setBounds(right.removeFromBottom(PARTIAL_SECTION_HEIGHT));

    // Toggle above the display
    auto toggleArea = right.removeFromTop(TOGGLE_HEIGHT);
    viewToggle.setBounds(toggleArea.reduced(4, 2));

    auto displayArea = right.reduced(2);
    waveformDisplay.setBounds(displayArea);
    frequencyDisplay.setBounds(displayArea);
}
