#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
#include <algorithm>

#include "AddSynth/AddSynthConstants.h"

class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay() { startTimerHz(30); }

    void updateCoefficients(const float* amps, const float* ratios, int numPartials)
    {
        const int N = static_cast<int>(waveform.size());
        for (int n = 0; n < N; ++n)
        {
            float sample = 0.0f;
            const float phase = 2*M_PI * static_cast<float>(n) / static_cast<float>(N);
            for (int i = 0; i < numPartials; ++i)
                sample += amps[i] * std::sin(phase * ratios[i]);
            waveform[n] = sample;
        }

        // Normalise for display
        float peak = 0.0f;
        for (auto v : waveform) peak = std::max(peak, std::abs(v));
        if (peak > 0.0f)
        {
            for (auto& s : waveform) s /= peak;
        }

        needsRepaint = true;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        const float midY  = bounds.getCentreY();
        const float halfH = bounds.getHeight() * 0.45f;

        g.fillAll(juce::Colour(0xff1c1b19));

        g.setColour(juce::Colour(0xff393836));
        g.drawHorizontalLine((int)midY, bounds.getX(), bounds.getRight());

        juce::Path path;
        const int N = static_cast<int>(waveform.size());
        for (int n = 0; n < N; ++n)
        {
            float x = bounds.getX() + bounds.getWidth() * static_cast<float>(n) / static_cast<float>(N - 1);
            float y = midY - waveform[n] * halfH;
            if (n == 0) path.startNewSubPath(x, y);
            else        path.lineTo(x, y);
        }

        g.setColour(juce::Colour(0xff4f98a3));
        g.strokePath(path, juce::PathStrokeType(1.5f));

        path.lineTo(bounds.getRight(), midY);
        path.lineTo(bounds.getX(), midY);
        path.closeSubPath();
        g.setColour(juce::Colour(0xff4f98a3).withAlpha(0.08f));
        g.fillPath(path);
    }

    void timerCallback() override
    {
        if (needsRepaint) { repaint(); needsRepaint = false; }
    }

private:
    std::array<float, 512> waveform {};
    bool needsRepaint = false;
};



class FrequencyDisplay : public juce::Component, private juce::Timer
{
public:
    FrequencyDisplay() { startTimerHz(30); }


    void updateCoefficients(const float* amps, const float* ratios, int numPartials)
    {
        juce::ignoreUnused(ratios);

        const int maxN = static_cast<int>(partialsCoef.size());
        std::fill(partialsCoef.begin(), partialsCoef.end(), 0.0f);
        numActive = std::clamp(numPartials, 0, maxN);
        if (numActive <= 0 || !amps) { needsRepaint = true; return; }

        for (int i = 0; i < numActive; ++i)
            partialsCoef[static_cast<size_t>(i)] = std::abs(amps[i]);

        // Convert to dB relative to peak and map to 0..1 for rendering
        const float eps = 1e-12f;
        float peak = 0.0f;
        for (int i = 0; i < numActive; ++i)
            peak = std::max(peak, partialsCoef[static_cast<size_t>(i)]);
        if (peak <= eps)
        {
            std::fill(partialsCoef.begin(), partialsCoef.end(), 0.0f);
            needsRepaint = true;
            return;
        }

        const float floorDb = -60.0f;
        const float invRange = 1.0f / (-floorDb);
        for (int i = 0; i < numActive; ++i)
        {
            // linear -> dB relative to peak
            const float lin = std::max(partialsCoef[static_cast<size_t>(i)], eps);
            const float db = 20.0f * std::log10(lin / peak);
            // map floorDb..0 -> 0..1
            float scaled = (db - floorDb) * invRange;
            partialsCoef[static_cast<size_t>(i)] = std::clamp(scaled, 0.0f, 1.0f);
        }
        needsRepaint = true;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        const float midY  = bounds.getCentreY();
        const float halfH = bounds.getHeight() * 0.45f;

        g.fillAll(juce::Colour(0xff1c1b19));

        // Draw baseline
        g.setColour(juce::Colour(0xff393836));
        g.drawHorizontalLine((int)midY, bounds.getX(), bounds.getRight());

        // Draw one bar per active partial, spanning the full usable width
        if (numActive <= 0) return;
        const float w = bounds.getWidth() / static_cast<float>(numActive);
        const float barW = std::max(1.0f, w * 0.9f);
        g.setColour(juce::Colour(0xff4f98a3));
        for (int i = 0; i < numActive; ++i)
        {
            const float x = bounds.getX() + i * w;
            const float h = partialsCoef[static_cast<size_t>(i)] * halfH;
            g.fillRect(juce::Rectangle<float>(x, midY - h, barW, h));
        }
    }

    void timerCallback() override
    {
        if (needsRepaint) { repaint(); needsRepaint = false; }
    }

private:
    std::array<float, 512> partialsCoef {};
    int  numActive = 0;
    bool needsRepaint = false;
};



// Magnitude response of the 2-pole low-pass state-variable filter (cutoff + Q).
class FilterShapeDisplay : public juce::Component
{
public:
    void setParams(float cutoffHz, float resonance)
    {
        if (cutoff == cutoffHz && reso == resonance) return;
        cutoff = cutoffHz;
        reso   = resonance;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.fillAll(juce::Colour(0xff1c1b19));

        constexpr float fMin = 20.0f, fMax = 20000.0f;
        const float logMin = std::log10(fMin);
        const float logMax = std::log10(fMax);

        constexpr float topDb = 18.0f, botDb = -36.0f;
        auto dbToY = [&](float db)
        {
            const float t = (db - botDb) / (topDb - botDb);
            return bounds.getBottom() - t * bounds.getHeight();
        };

        // 0 dB reference line
        g.setColour(juce::Colour(0xff393836));
        g.drawHorizontalLine((int)dbToY(0.0f), bounds.getX(), bounds.getRight());

        const float Q  = std::max(0.1f, reso);
        const float fc = std::clamp(cutoff, fMin, fMax);
        const int   N  = std::max(1, (int)bounds.getWidth());

        juce::Path path;
        for (int px = 0; px <= N; ++px)
        {
            const float t    = static_cast<float>(px) / static_cast<float>(N);
            const float f    = std::pow(10.0f, logMin + t * (logMax - logMin));
            const float w    = f / fc;            // normalised frequency
            const float w2   = w * w;
            const float den  = std::sqrt((1.0f - w2) * (1.0f - w2) + w2 / (Q * Q));
            const float mag  = (den > 1e-9f) ? 1.0f / den : 1e9f;
            const float db   = 20.0f * std::log10(std::max(mag, 1e-9f));
            const float x    = bounds.getX() + static_cast<float>(px);
            const float y    = std::clamp(dbToY(db), bounds.getY(), bounds.getBottom());
            if (px == 0) path.startNewSubPath(x, y);
            else         path.lineTo(x, y);
        }

        juce::Path fill = path;
        fill.lineTo(bounds.getRight(), bounds.getBottom());
        fill.lineTo(bounds.getX(),     bounds.getBottom());
        fill.closeSubPath();
        g.setColour(juce::Colour(0xff4f98a3).withAlpha(0.08f));
        g.fillPath(fill);

        g.setColour(juce::Colour(0xff4f98a3));
        g.strokePath(path, juce::PathStrokeType(1.5f));

        // cutoff marker
        const float tC = (std::log10(fc) - logMin) / (logMax - logMin);
        const float xC = bounds.getX() + tC * bounds.getWidth();
        g.setColour(juce::Colour(0xff6bb3be).withAlpha(0.45f));
        g.drawVerticalLine((int)xC, bounds.getY(), bounds.getBottom());
    }

private:
    float cutoff { 2000.0f };
    float reso   { 0.7f };
};



// ADSR envelope shape (attack/decay in ms, sustain 0..1, release in ms).
// Always draws the global envelope plus all DSP::kNumBands per-band envelopes overlaid
// in the same graph, each band colour-coded, with a small legend.
class ADSRDisplay : public juce::Component
{
public:
    static juce::Colour bandColour(int b)
    {
        const float hue = 0.55f - 0.45f * (static_cast<float>(b) / std::max(1, DSP::kNumBands - 1));
        return juce::Colour::fromHSV(hue, 0.55f, 0.95f, 1.0f);
    }

    void setParams(float attackMs, float decayMs, float sustainLevel, float releaseMs)
    {
        if (attack == attackMs && decay == decayMs && sustain == sustainLevel && release == releaseMs)
            return;
        attack  = attackMs;
        decay   = decayMs;
        sustain = sustainLevel;
        release = releaseMs;
        repaint();
    }

    void setBandParams(const std::array<float, DSP::kNumBands>& a,
                       const std::array<float, DSP::kNumBands>& d,
                       const std::array<float, DSP::kNumBands>& s,
                       const std::array<float, DSP::kNumBands>& r)
    {
        if (a == bandA && d == bandD && s == bandS && r == bandR)
            return;
        bandA = a; bandD = d; bandS = s; bandR = r;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1c1b19));

        auto r = getLocalBounds().toFloat().reduced(6.0f);
        g.setColour(juce::Colour(0xff393836));
        g.drawHorizontalLine((int)r.getBottom(), r.getX(), r.getRight());

        // per-band envelopes, overlaid, hue ramped low->high
        for (int b = 0; b < DSP::kNumBands; ++b)
            drawEnvelope(g, r, bandA[(size_t)b], bandD[(size_t)b], bandS[(size_t)b], bandR[(size_t)b],
                         bandColour(b), false);

        // global envelope on top, neutral colour + light fill so it stands out
        drawEnvelope(g, r, attack, decay, sustain, release, juce::Colour(0xffe0ddd8), true);

        drawLegend(g, getLocalBounds().toFloat());
    }

private:
    static void drawLegend(juce::Graphics& g, juce::Rectangle<float> area)
    {
        const float sw = 8.0f, gap = 3.0f, rowH = 11.0f;
        float x = area.getX() + 6.0f;
        const float y = area.getY() + 4.0f;
        g.setFont(juce::FontOptions(9.0f, juce::Font::plain));

        auto entry = [&](juce::Colour c, const juce::String& label, float labelW)
        {
            g.setColour(c);
            g.fillRect(x, y + (rowH - sw) * 0.5f, sw, sw);
            x += sw + 2.0f;
            g.setColour(juce::Colour(0xff8a8580));
            g.drawText(label, juce::Rectangle<float>(x, y, labelW, rowH), juce::Justification::centredLeft);
            x += labelW + gap;
        };

        entry(juce::Colour(0xffe0ddd8), "Global", 34.0f);
        for (int b = 0; b < DSP::kNumBands; ++b)
            entry(bandColour(b), "B" + juce::String(b + 1), 16.0f);
    }

    static void drawEnvelope(juce::Graphics& g, juce::Rectangle<float> r,
                             float attackMs, float decayMs, float sustainLevel, float releaseMs,
                             juce::Colour colour, bool withFill)
    {
        const float top  = r.getY();
        const float bot  = r.getBottom();
        const float left = r.getX();
        const float w    = r.getWidth();

        constexpr float sustainFrac = 0.22f;          // fixed display width for the sustain hold
        const float sum = std::max(1.0f, attackMs + decayMs + releaseMs);
        const float adsrW = w * (1.0f - sustainFrac);
        const float ax = adsrW * (attackMs  / sum);
        const float dx = adsrW * (decayMs   / sum);
        const float rx = adsrW * (releaseMs / sum);
        const float sx = w * sustainFrac;

        const float susY = bot - std::clamp(sustainLevel, 0.0f, 1.0f) * (bot - top);

        juce::Path p;
        float x = left;
        p.startNewSubPath(x, bot);
        x += ax; p.lineTo(x, top);    // attack
        x += dx; p.lineTo(x, susY);   // decay
        x += sx; p.lineTo(x, susY);   // sustain hold
        x += rx; p.lineTo(x, bot);    // release

        if (withFill)
        {
            juce::Path fill = p;
            fill.lineTo(x, bot);
            fill.lineTo(left, bot);
            fill.closeSubPath();
            g.setColour(colour.withAlpha(0.08f));
            g.fillPath(fill);
        }

        g.setColour(colour);
        g.strokePath(p, juce::PathStrokeType(1.5f));
    }

    float attack  { 10.0f };
    float decay   { 80.0f };
    float sustain { 0.7f };
    float release { 120.0f };

    std::array<float, DSP::kNumBands> bandA {};
    std::array<float, DSP::kNumBands> bandD {};
    std::array<float, DSP::kNumBands> bandS {};
    std::array<float, DSP::kNumBands> bandR {};
};
