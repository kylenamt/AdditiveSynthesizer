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
static constexpr int BAND_ENV_HEIGHT { 230 };

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

// Compact grid of per-band ADSR knobs (5 band columns x A/D/S/R rows) plus a row of
// crossover knobs. Mirrors the attachment pattern used by PartialKnobArray.
class BandEnvPanel final : public juce::Component
{
public:
    explicit BandEnvPanel(mrta::ParameterManager& parameterManager) :
        apvts(parameterManager.getAPVTS())
    {
        attachments.reserve(static_cast<size_t>(DSP::kNumBands) * 4 + DSP::kNumCrossovers);

        for (int b = 0; b < DSP::kNumBands; ++b)
        {
            auto& bl = bandLabels[static_cast<size_t>(b)];
            bl.setText("B" + juce::String(b + 1), juce::dontSendNotification);
            bl.setJustificationType(juce::Justification::centred);
            bl.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            addAndMakeVisible(bl);

            for (int stage = 0; stage < 4; ++stage)
            {
                auto& knob = stageKnobs[static_cast<size_t>(b * 4 + stage)];
                knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 12);
                addAndMakeVisible(knob);
                attachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    apvts, Param::bandParamId(b, stage), knob));
            }
        }

        static const char* stageNames[4] { "A", "D", "S", "R" };
        for (int stage = 0; stage < 4; ++stage)
        {
            auto& sl = stageLabels[static_cast<size_t>(stage)];
            sl.setText(stageNames[stage], juce::dontSendNotification);
            sl.setJustificationType(juce::Justification::centred);
            sl.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            addAndMakeVisible(sl);
        }

        for (int i = 0; i < DSP::kNumCrossovers; ++i)
        {
            auto& knob = crossoverKnobs[static_cast<size_t>(i)];
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 12);
            addAndMakeVisible(knob);
            attachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, Param::bandCrossoverId(i), knob));

            auto& xl = crossoverLabels[static_cast<size_t>(i)];
            xl.setText("X" + juce::String(i + 1), juce::dontSendNotification);
            xl.setJustificationType(juce::Justification::centred);
            xl.setFont(juce::FontOptions(9.0f, juce::Font::plain));
            addAndMakeVisible(xl);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(2);

        // Bottom strip: crossover knobs spanning the width.
        auto xoverArea = bounds.removeFromBottom(56);
        const int xCell = xoverArea.getWidth() / DSP::kNumCrossovers;
        for (int i = 0; i < DSP::kNumCrossovers; ++i)
        {
            auto cell = juce::Rectangle<int>(xoverArea.getX() + i * xCell, xoverArea.getY(), xCell, xoverArea.getHeight())
                            .reduced(3);
            crossoverLabels[static_cast<size_t>(i)].setBounds(cell.removeFromTop(10));
            crossoverKnobs[static_cast<size_t>(i)].setBounds(cell);
        }

        // Header row of band labels.
        const int stageColW = 18;
        auto header = bounds.removeFromTop(12);
        header.removeFromLeft(stageColW);
        const int bandColW = header.getWidth() / DSP::kNumBands;
        for (int b = 0; b < DSP::kNumBands; ++b)
            bandLabels[static_cast<size_t>(b)].setBounds(header.getX() + b * bandColW, header.getY(), bandColW, 12);

        // Grid: 4 stage rows x kNumBands columns, with a stage label column on the left.
        const int rowH = bounds.getHeight() / 4;
        for (int stage = 0; stage < 4; ++stage)
        {
            auto row = juce::Rectangle<int>(bounds.getX(), bounds.getY() + stage * rowH, bounds.getWidth(), rowH);
            stageLabels[static_cast<size_t>(stage)].setBounds(row.removeFromLeft(stageColW));
            const int cellW = row.getWidth() / DSP::kNumBands;
            for (int b = 0; b < DSP::kNumBands; ++b)
                stageKnobs[static_cast<size_t>(b * 4 + stage)]
                    .setBounds(juce::Rectangle<int>(row.getX() + b * cellW, row.getY(), cellW, rowH).reduced(2));
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::array<juce::Slider, static_cast<size_t>(DSP::kNumBands) * 4> stageKnobs {};
    std::array<juce::Slider, DSP::kNumCrossovers> crossoverKnobs {};
    std::array<juce::Label, DSP::kNumBands> bandLabels {};
    std::array<juce::Label, 4> stageLabels {};
    std::array<juce::Label, DSP::kNumCrossovers> crossoverLabels {};
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
    return { Param::ID::EnvMode,
             Param::ID::Attack,
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

    bandEnvPanel = std::make_unique<BandEnvPanel>(processor.getParameterManager());

    // Accordion of collapsible parameter sections
    addAndMakeVisible(concertina);
    addSection(mainEditor,      headerMain,      "MAIN",          static_cast<int>(makeMainParams().size()),      true);
    addSection(spectrumEditor,  headerSpectrum,  "SPECTRUM",      static_cast<int>(makeSpectrumParams().size()),  false);
    addSection(envEditor,       headerEnv,       "ENVELOPE",      static_cast<int>(makeEnvParams().size()),       false);
    addComponentSection(*bandEnvPanel, headerBandEnv, "BAND ENV", BAND_ENV_HEIGHT, false);
    addSection(filterEnvEditor, headerFilterEnv, "FILTER ENV",    static_cast<int>(makeFilterEnvParams().size()), false);
    addSection(filterLfoEditor, headerFilterLfo, "FILTER + LFO",  static_cast<int>(makeFilterLfoParams().size()), false);

    // visualisation displays + tab selector
    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(frequencyDisplay);
    addAndMakeVisible(filterDisplay);
    addAndMakeVisible(adsrDisplay);

    const auto tabBg = juce::Colour(CustomLookAndFeel::colSurface);
    viewTabs.addTab("Wave",     tabBg, TabWave);
    viewTabs.addTab("Spectrum", tabBg, TabSpectrum);
    viewTabs.addTab("Filter",   tabBg, TabFilter);
    viewTabs.addTab("ADSR",     tabBg, TabAdsr);
    viewTabs.setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(CustomLookAndFeel::colBorder));
    viewTabs.setColour(juce::TabbedButtonBar::frontTextColourId,  juce::Colour(CustomLookAndFeel::colTextPrimary));
    viewTabs.setColour(juce::TabbedButtonBar::tabTextColourId,    juce::Colour(CustomLookAndFeel::colTextSecondary));
    viewTabs.setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colour(CustomLookAndFeel::colAccent));
    viewTabs.onChange = [this] { updateDisplayVisibility(); };
    addAndMakeVisible(viewTabs);
    viewTabs.setCurrentTabIndex(TabWave);

    partialKnobs = std::make_unique<PartialKnobArray>(processor.getParameterManager());
    addAndMakeVisible(*partialKnobs);

    // Window tall enough to fully show the largest section plus the other collapsed headers.
    const int largestParamSection = static_cast<int>(std::max({ makeMainParams().size(), makeSpectrumParams().size(), makeEnvParams().size(), makeFilterEnvParams().size(), makeFilterLfoParams().size() }));
    const int largestSectionHeight = std::max(largestParamSection * PARAM_HEIGHT, BAND_ENV_HEIGHT);
    const int accordionHeight = largestSectionHeight + 6 * HEADER_HEIGHT;
    const int rightHeight = TOGGLE_HEIGHT + PARTIAL_SECTION_HEIGHT + 200; // display + knobs
    const int contentHeight = std::max(accordionHeight, rightHeight);
    setSize(MARGIN * 2 + ACCORDION_WIDTH + COLUMN_GAP + 430, contentHeight + MARGIN * 2);

    // initialize displays from current processor state and start timer to poll updates
    waveformDisplay.updateCoefficients(processor.getBlendedBuffer().data(), processor.getRatios().data(), processor.getNumPartials());
    frequencyDisplay.updateCoefficients(processor.getBlendedBuffer().data(), processor.getRatios().data(), processor.getNumPartials());
    startTimerHz(60);
    updateDisplayVisibility();
}

void AdditiveSynthesizerEditor::addSection(mrta::GenericParameterEditor& editor,
                                           std::unique_ptr<SectionHeader>& header,
                                           const juce::String& title, int paramCount,
                                           bool startExpanded)
{
    addComponentSection(editor, header, title, paramCount * PARAM_HEIGHT, startExpanded);
}

void AdditiveSynthesizerEditor::addComponentSection(juce::Component& content,
                                                    std::unique_ptr<SectionHeader>& header,
                                                    const juce::String& title, int contentHeight,
                                                    bool startExpanded)
{
    header = std::make_unique<SectionHeader>(title);

    concertina.addPanel(-1, &content, false);
    concertina.setPanelHeaderSize(&content, HEADER_HEIGHT);
    concertina.setCustomPanelHeader(&content, header.get(), false);
    concertina.setMaximumPanelSize(&content, contentHeight);

    auto* headerPtr = header.get();
    header->onToggle = [this, panel = &content, headerPtr, contentHeight]
    {
        togglePanel(panel, headerPtr, contentHeight);
    };

    header->setExpanded(startExpanded);
    concertina.setPanelSize(&content, startExpanded ? contentHeight : 0, false);
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

    // Push current parameter values into the filter / envelope shape displays.
    auto& apvts = processor.getParameterManager().getAPVTS();
    auto val = [&apvts](const juce::String& id) -> float
    {
        if (auto* p = apvts.getRawParameterValue(id))
            return p->load();
        return 0.0f;
    };

    filterDisplay.setParams(val(Param::ID::FilterCutoff), val(Param::ID::FilterReso));

    // Always feed the ADSR view both the global envelope and all per-band envelopes;
    // they are drawn overlaid in the same graph, colour-coded per band.
    adsrDisplay.setParams(val(Param::ID::Attack), val(Param::ID::Decay),
                          val(Param::ID::Sustain), val(Param::ID::Release));

    std::array<float, DSP::kNumBands> a {}, d {}, s {}, r {};
    for (int b = 0; b < DSP::kNumBands; ++b)
    {
        a[(size_t)b] = val(Param::bandParamId(b, 0));
        d[(size_t)b] = val(Param::bandParamId(b, 1));
        s[(size_t)b] = val(Param::bandParamId(b, 2));
        r[(size_t)b] = val(Param::bandParamId(b, 3));
    }
    adsrDisplay.setBandParams(a, d, s, r);
}

void AdditiveSynthesizerEditor::updateDisplayVisibility()
{
    const int tab = viewTabs.getCurrentTabIndex();
    waveformDisplay.setVisible(tab == TabWave);
    frequencyDisplay.setVisible(tab == TabSpectrum);
    filterDisplay.setVisible(tab == TabFilter);
    adsrDisplay.setVisible(tab == TabAdsr);
    repaint();
}

AdditiveSynthesizerEditor::~AdditiveSynthesizerEditor()
{
    // Stop the polling timer before any members are torn down so a late
    // timerCallback() can never touch half-destroyed state.
    stopTimer();
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

    // Tab selector above the display
    auto tabArea = right.removeFromTop(TOGGLE_HEIGHT);
    viewTabs.setBounds(tabArea);

    auto displayArea = right.reduced(2);
    waveformDisplay.setBounds(displayArea);
    frequencyDisplay.setBounds(displayArea);
    filterDisplay.setBounds(displayArea);
    adsrDisplay.setBounds(displayArea);
}
