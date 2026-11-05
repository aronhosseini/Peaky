#pragma once
#include "PluginProcessor.h"
#include "PeakyLookAndFeel.h"

// Multi-layer waveform analyzer (~4 s history): electric-teal -> slate gradient, coral gain-reduction curtain,
// backlit coral PEAK THRESHOLD / TARGET LEVEL lines. Scale: 0 dB (top) ... -48 dB (bottom).
class ScopeDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ScopeDisplay (PeakyAudioProcessor& p) : proc (p)
    {
        thr = proc.apvts.getRawParameterValue (IDs::threshold);
        tgt = proc.apvts.getRawParameterValue (IDs::target);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace PeakyColours;
        constexpr int K = peaky::PeakySmoother::kHistoryColumns;
        auto b = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff10141a), 0, 0, juce::Colour (0xff07090c), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 6.f);

        const auto plot = b.reduced (2.f).withTrimmedLeft (34.f);
        const float top = plot.getY() + 4.f, bottom = plot.getBottom() - 4.f, h = bottom - top;
        auto dbToY = [=] (float db) { return top + (-juce::jlimit (-48.f, 0.f, db) / 48.f) * h; };

        // grid + vertical scale
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        for (int db = 0; db >= -48; db -= 6)
        {
            const float y = dbToY ((float) db);
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
            g.setColour (dim);
            g.drawText (juce::String (db) + "dB", (int) b.getX() + 2, (int) y - 6, 32, 12, juce::Justification::centredRight);
        }

        // history snapshot (oldest -> newest)
        const int head = proc.getActiveDsp().histHead.load (std::memory_order_acquire);
        std::array<float, K> db {}, sm {}, gr {};
        for (int c = 0; c < K; ++c)
        {
            const int idx = (head + c) % K;
            db[(size_t) c] = peaky::PeakySmoother::toDb (proc.getActiveDsp().histPeak[idx].load (std::memory_order_relaxed));
            gr[(size_t) c] = proc.getActiveDsp().histGr[idx].load (std::memory_order_relaxed);
        }
        for (int c = 0; c < K; ++c)   // 9-column box blur for the inner layer
        {
            float s = 0.f; int n = 0;
            for (int k = -4; k <= 4; ++k) { const int j = c + k; if (j >= 0 && j < K) { s += db[(size_t) j]; ++n; } }
            sm[(size_t) c] = s / (float) n - 2.f;
        }
        auto xAt = [&] (int c) { return plot.getX() + plot.getWidth() * (float) c / (float) (K - 1); };

        auto makeFill = [&] (const std::array<float, K>& levels, juce::Path& area, juce::Path* edgeLine)
        {
            area.startNewSubPath (plot.getX(), bottom);
            for (int c = 0; c < K; ++c)
            {
                const float y = dbToY (levels[(size_t) c]);
                area.lineTo (xAt (c), y);
                if (edgeLine) (c == 0 ? edgeLine->startNewSubPath (xAt (c), y) : edgeLine->lineTo (xAt (c), y));
            }
            area.lineTo (plot.getRight(), bottom);
            area.closeSubPath();
        };
        auto gradient = [&] (float opacity)
        {
            juce::ColourGradient grad (juce::Colour (0xff19e8f9), 0, bottom, juce::Colour (0xff1a202b), 0, top, false);
            grad.addColour (0.30, juce::Colour (0xff1a93bd));
            grad.addColour (0.62, juce::Colour (0xff2c4d86));
            grad.multiplyOpacity (opacity);
            return grad;
        };

        juce::Path outer, outerEdge, inner;
        makeFill (db, outer, &outerEdge);
        makeFill (sm, inner, nullptr);

        g.setGradientFill (gradient (0.50f));
        g.fillPath (outer);
        g.setGradientFill (gradient (0.95f));
        g.fillPath (inner);

        g.saveState();                    // fine scan-line texture inside the waveform
        g.reduceClipRegion (outer);
        g.setColour (juce::Colours::black.withAlpha (0.14f));
        for (float y = top; y < bottom; y += 3.f) g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.restoreState();

        g.setColour (teal.withAlpha (0.35f));
        g.strokePath (outerEdge, juce::PathStrokeType (3.f));
        g.setColour (juce::Colour (0xffbff8ff).withAlpha (0.85f));
        g.strokePath (outerEdge, juce::PathStrokeType (1.1f));

        // gain reduction curtain hanging from the top
        juce::Path red, redEdge;
        red.startNewSubPath (plot.getX(), top);
        for (int c = 0; c < K; ++c)
        {
            const float y = top + juce::jlimit (0.f, 48.f, -gr[(size_t) c]) / 48.f * h;
            red.lineTo (xAt (c), y);
            (c == 0 ? redEdge.startNewSubPath (xAt (c), y) : redEdge.lineTo (xAt (c), y));
        }
        red.lineTo (plot.getRight(), top);
        red.closeSubPath();
        g.setGradientFill (juce::ColourGradient (coral.withAlpha (0.55f), 0, top, coral.withAlpha (0.08f), 0, top + h * 0.30f, false));
        g.fillPath (red);
        g.setColour (coral.withAlpha (0.30f));
        g.strokePath (redEdge, juce::PathStrokeType (3.f));
        g.setColour (juce::Colour (0xffffb1a4));
        g.strokePath (redEdge, juce::PathStrokeType (1.2f));

        // backlit coral reference lines
        const float yThr = dbToY (thr->load()), yTgt = dbToY (tgt->load());
        auto line = [&] (float y, const char* label, bool labelAbove)
        {
            g.setColour (coral.withAlpha (0.14f));
            g.drawLine (plot.getX(), y, plot.getRight(), y, 6.f);
            g.setColour (coral.withAlpha (0.28f));
            g.drawLine (plot.getX(), y, plot.getRight(), y, 3.f);
            g.setColour (juce::Colour (0xffffa797));
            g.drawLine (plot.getX(), y, plot.getRight(), y, 1.4f);
            g.setFont (juce::Font (juce::FontOptions (10.f, juce::Font::bold).withKerningFactor (0.06f)));
            g.setColour (juce::Colour (0xffffb8aa));
            g.drawText (label, (int) plot.getX() + 8, (int) (labelAbove ? y - 14.f : y + 3.f), 150, 11, juce::Justification::left);
        };
        const bool thrUpper = yThr <= yTgt;
        line (yThr, "PEAK THRESHOLD", thrUpper);
        line (yTgt, "TARGET LEVEL", ! thrUpper);

        g.setColour (juce::Colour (0xff2c3a46));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 1.2f);
    }

private:
    void timerCallback() override { repaint(); }
    PeakyAudioProcessor& proc;
    std::atomic<float> *thr, *tgt;
};

// Vertical gain-reduction meter: 24 one-dB segments (0 .. -24 dB), peak-hold marker.
class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    explicit GainReductionMeter (PeakyAudioProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        using namespace PeakyColours;
        const float top = 4.f, bottom = (float) getHeight() - 4.f;
        constexpr int segs = 24;
        const float segH = (bottom - top) / (float) segs, barX = 40.f, barW = 14.f;

        g.setFont (juce::Font (juce::FontOptions (10.f, juce::Font::bold)));
        for (int k : { 0, 4, 8, 12, 16, 20, 24 })
        {
            g.setColour (dim);
            g.drawText (k == 0 ? "0dB" : "-" + juce::String (k) + "dB", 0, (int) (top + (float) k * segH) - 6, 36, 12, juce::Justification::centredRight);
        }
        for (int i = 0; i < segs; ++i)
        {
            const float y = top + (float) i * segH;
            const bool lit = shown > (float) i + 0.15f;
            const auto r = juce::Rectangle<float> (barX, y + 0.6f, barW, segH - 1.2f);
            if (lit)
            {
                const auto c = coral.interpolatedWith (juce::Colour (0xff9a5a8c), (float) i / (float) segs);
                g.setColour (c.withAlpha (0.28f));
                g.fillRoundedRectangle (r.expanded (1.5f, 0.5f), 2.f);
                g.setColour (c);
            }
            else
                g.setColour (juce::Colour (0xff141b22).interpolatedWith (juce::Colour (0xff0e1a20), (float) i / (float) segs));
            g.fillRoundedRectangle (r, 1.6f);
        }
        const float hy = top + juce::jlimit (0.f, 24.f, hold) * segH;
        juce::Path tri;
        tri.addTriangle (barX + barW + 8.f, hy - 4.5f, barX + barW + 8.f, hy + 4.5f, barX + barW + 2.f, hy);
        g.setColour (juce::Colour (0xffe9ebef));
        g.fillPath (tri);
    }

private:
    void timerCallback() override
    {
        const float mag = -proc.getActiveDsp().grMeterDb.load (std::memory_order_relaxed);
        shown = mag > shown ? mag : juce::jmax (mag, shown - 0.7f);
        if (mag >= hold) { hold = mag; holdFrames = 45; }
        else if (--holdFrames <= 0) hold = juce::jmax (shown, hold - 0.3f);
        repaint();
    }
    PeakyAudioProcessor& proc;
    float shown = 0.f, hold = 0.f;
    int holdFrames = 0;
};
