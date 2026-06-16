#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Color palette
    static constexpr juce::uint32 colBackground   = 0xff1c1b19;
    static constexpr juce::uint32 colSurface      = 0xff2a2826;
    static constexpr juce::uint32 colBorder       = 0xff393836;
    static constexpr juce::uint32 colAccent       = 0xff4f98a3;
    static constexpr juce::uint32 colAccentHover  = 0xff6bb3be;
    static constexpr juce::uint32 colTextPrimary  = 0xffe0ddd8;
    static constexpr juce::uint32 colTextSecondary = 0xff8a8580;
    static constexpr juce::uint32 colKnobBody     = 0xff3a3836;

    CustomLookAndFeel()
    {
        // Window / panel backgrounds
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(colBackground));

        // Sliders
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(colAccent));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(colKnobBody));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(colTextPrimary));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(colSurface));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(colBorder));
        setColour(juce::Slider::thumbColourId, juce::Colour(colAccent));
        setColour(juce::Slider::trackColourId, juce::Colour(colKnobBody));

        // Labels
        setColour(juce::Label::textColourId, juce::Colour(colTextPrimary));
        setColour(juce::Label::backgroundColourId, juce::Colour(0x00000000));
        setColour(juce::Label::outlineColourId, juce::Colour(0x00000000));

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(colSurface));
        setColour(juce::ComboBox::textColourId, juce::Colour(colTextPrimary));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(colBorder));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(colTextSecondary));
        setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(colAccent));

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(colSurface));
        setColour(juce::PopupMenu::textColourId, juce::Colour(colTextPrimary));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(colAccent));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(colTextPrimary));

        // TextButton
        setColour(juce::TextButton::buttonColourId, juce::Colour(colSurface));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(colAccent));
        setColour(juce::TextButton::textColourOffId, juce::Colour(colTextPrimary));
        setColour(juce::TextButton::textColourOnId, juce::Colour(colTextPrimary));

        // ToggleButton
        setColour(juce::ToggleButton::textColourId, juce::Colour(colTextPrimary));
        setColour(juce::ToggleButton::tickColourId, juce::Colour(colAccent));
        setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(colTextSecondary));

        // TextEditor (used in slider text boxes)
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(colSurface));
        setColour(juce::TextEditor::textColourId, juce::Colour(colTextPrimary));
        setColour(juce::TextEditor::outlineColourId, juce::Colour(colBorder));
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(colAccent));
        setColour(juce::TextEditor::highlightColourId, juce::Colour(colAccent).withAlpha(0.3f));

        // CaretComponent
        setColour(juce::CaretComponent::caretColourId, juce::Colour(colAccent));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        juce::ignoreUnused(slider);

        const float radius = static_cast<float>(juce::jmin(width, height)) * 0.4f;
        const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;

        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Value arc geometry
        const float arcRadius = radius;
        const float arcThickness = radius * 0.18f;

        // Background track (full sweep)
        juce::Path track;
        track.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(colKnobBody));
        g.strokePath(track, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // Filled value arc (start -> current position)
        if (sliderPosProportional > 0.0f)
        {
            juce::Path fill;
            fill.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, angle, true);
            g.setColour(juce::Colour(colAccent));
            g.strokePath(fill, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        // Filled circle body
        const float bodyRadius = radius * 0.72f;
        g.setColour(juce::Colour(colKnobBody));
        g.fillEllipse(centreX - bodyRadius, centreY - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

        // Subtle outer ring
        g.setColour(juce::Colour(colBorder));
        g.drawEllipse(centreX - bodyRadius, centreY - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

        // Notch/line indicator pointing to current value
        const float notchLength = bodyRadius * 0.5f;
        const float notchStartRadius = bodyRadius * 0.35f;

        juce::Path notch;
        notch.addRoundedRectangle(-1.5f, -notchStartRadius - notchLength, 3.0f, notchLength, 1.5f);
        notch.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        g.setColour(juce::Colour(colAccent));
        g.fillPath(notch);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH, isButtonDown);

        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
        const float cornerSize = 4.0f;

        g.setColour(juce::Colour(colSurface));
        g.fillRoundedRectangle(bounds, cornerSize);

        auto outlineColour = box.hasKeyboardFocus(true) ? juce::Colour(colAccent) : juce::Colour(colBorder);
        g.setColour(outlineColour);
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

        // Arrow
        const float arrowX = static_cast<float>(width) - 18.0f;
        const float arrowY = static_cast<float>(height) * 0.5f;
        juce::Path arrow;
        arrow.addTriangle(arrowX - 4.0f, arrowY - 2.0f,
                          arrowX + 4.0f, arrowY - 2.0f,
                          arrowX, arrowY + 3.0f);
        g.setColour(juce::Colour(colTextSecondary));
        g.fillPath(arrow);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const float cornerSize = 4.0f;

        auto baseColour = backgroundColour;
        if (isButtonDown)
            baseColour = baseColour.brighter(0.1f);
        else if (isMouseOverButton)
            baseColour = baseColour.brighter(0.05f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(button.getToggleState() ? juce::Colour(colAccent) : juce::Colour(colBorder));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsDown);

        auto bounds = button.getLocalBounds().toFloat();
        const float toggleWidth = 36.0f;
        const float toggleHeight = 18.0f;
        const float toggleX = bounds.getX() + 4.0f;
        const float toggleY = bounds.getCentreY() - toggleHeight * 0.5f;

        auto toggleBounds = juce::Rectangle<float>(toggleX, toggleY, toggleWidth, toggleHeight);
        const float cornerSize = toggleHeight * 0.5f;

        // Track
        if (button.getToggleState())
            g.setColour(juce::Colour(colAccent));
        else
            g.setColour(juce::Colour(colKnobBody));

        g.fillRoundedRectangle(toggleBounds, cornerSize);

        // Thumb
        const float thumbDiameter = toggleHeight - 4.0f;
        float thumbX = button.getToggleState()
            ? toggleBounds.getRight() - thumbDiameter - 2.0f
            : toggleBounds.getX() + 2.0f;

        g.setColour(juce::Colour(colTextPrimary));
        g.fillEllipse(thumbX, toggleY + 2.0f, thumbDiameter, thumbDiameter);

        // Label text
        auto textBounds = bounds.withLeft(toggleX + toggleWidth + 8.0f);
        g.setColour(shouldDrawButtonAsHighlighted ? juce::Colour(colTextPrimary) : juce::Colour(colTextSecondary));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(button.getButtonText(), textBounds.toNearestInt(), juce::Justification::centredLeft);
    }

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override
    {
        auto layout = LookAndFeel_V4::getSliderLayout(slider);

        if (slider.getSliderStyle() == juce::Slider::RotaryHorizontalVerticalDrag)
        {
            if (slider.getTextBoxPosition() == juce::Slider::TextBoxBelow)
            {
                auto bounds = slider.getLocalBounds();
                layout.textBoxBounds = bounds.removeFromBottom(16).reduced(4, 0);
                layout.sliderBounds = bounds.reduced(2);
            }
        }

        return layout;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
};
