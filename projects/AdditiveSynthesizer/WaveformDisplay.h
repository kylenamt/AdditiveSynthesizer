#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
#include <algorithm>

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
        const int N = static_cast<int>(partialsCoef.size());
        std::fill(partialsCoef.begin(), partialsCoef.end(), 0.0f);
        if (numPartials <= 0 || !ratios || !amps) return;

        // Determine ratio range (use positive ratios only)
        float minR = std::numeric_limits<float>::infinity();
        float maxR = 0.0f;
        for (int i = 0; i < numPartials; ++i)
        {
            const float r = ratios[i];
            if (r > 0.0f)
            {
                minR = std::min(minR, r);
                maxR = std::max(maxR, r);
            }
        }
        if (!std::isfinite(minR) || maxR <= 0.0f)
            return;

        const float logMin = std::log(minR);
        const float logMax = std::log(maxR);
        const float denom = (logMax > logMin) ? (logMax - logMin) : 1.0f;

        for (int n = 0; n < numPartials; ++n)
        {
            const float r = std::max(1e-6f, ratios[n]);
            const float logR = std::log(r);
            const float t = (logR - logMin) / denom; // 0..1
            const int idx = std::clamp(static_cast<int>(std::round(t * (N - 1))), 0, N - 1);
            partialsCoef[static_cast<size_t>(idx)] += amps[n];
        }

        // Convert to dB relative to peak and map to 0..1 for rendering
        const float eps = 1e-12f;
        float peak = 0.0f;
        for (auto v : partialsCoef) peak = std::max(peak, v);
        if (peak <= eps)
        {
            std::fill(partialsCoef.begin(), partialsCoef.end(), 0.0f);
            needsRepaint = true;
            return;
        }

        const float floorDb = -60.0f;
        const float invRange = 1.0f / (-floorDb);
        for (auto& s : partialsCoef)
        {
            // linear -> dB relative to peak
            const float lin = std::max(s, eps);
            const float db = 20.0f * std::log10(lin / peak);
            // map floorDb..0 -> 0..1
            float scaled = (db - floorDb) * invRange;
            scaled = std::clamp(scaled, 0.0f, 1.0f);
            s = scaled;
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

        // Draw bars for partials
        const int N = static_cast<int>(partialsCoef.size());
        if (N == 0) return;
        const float w = bounds.getWidth() / static_cast<float>(N);
        g.setColour(juce::Colour(0xff4f98a3));
        for (int i = 0; i < N; ++i)
        {
            const float x = bounds.getX() + i * w;
            const float h = partialsCoef[static_cast<size_t>(i)] * halfH;
            g.fillRect(juce::Rectangle<float>(x, midY - h, w * 0.9f, h));
        }
    }

    void timerCallback() override
    {
        if (needsRepaint) { repaint(); needsRepaint = false; }
    }

private:
    std::array<float, 512> partialsCoef {};
    bool needsRepaint = false;
};
