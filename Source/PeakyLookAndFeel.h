#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace PeakyColours
{
const juce::Colour teal    { 0xff35e0ee };   // electric teal accent
const juce::Colour tealDim { 0xff1f8a96 };
const juce::Colour coral   { 0xffff7f6e };   // reference lines / gain reduction
const juce::Colour arcBlue { 0xff2f5fd6 };   // knob value ring
const juce::Colour panel   { 0xff0b171d };
const juce::Colour edge    { 0xff2a4752 };
const juce::Colour text    { 0xffd4d7dd };
const juce::Colour dim     { 0xff9a9fa9 };
const juce::Colour bgTop   { 0xff25262b };
const juce::Colour bgBot   { 0xff111215 };

inline float hash01 (int i) { float h = std::sin ((float) i * 12.9898f) * 43758.5453f; return h - std::floor (h); }
}

class PeakyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PeakyLookAndFeel()
    {
        using namespace PeakyColours;
        setColour (juce::TextButton::textColourOnId, teal);
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffcfd2d8));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xffdcdfe4));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1c20));
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, tealDim);
        setColour (juce::PopupMenu::headerTextColourId, dim);
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1b1c20));
        setColour (juce::AlertWindow::textColourId, text);
        setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xffe9ebef));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        using namespace PeakyColours;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
        const float d = juce::jmin (b.getWidth(), b.getHeight());
        const auto c = b.getCentre();
        const float radius = d * 0.5f, arcR = radius - 3.f, bezelR = radius - 8.f, capR = bezelR * 0.80f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        // glowing accent ring
        juce::Path track, value;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startAngle, endAngle, true);
        value.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startAngle, angle, true);
        g.setColour (juce::Colour (0xff161b28));
        g.strokePath (track, juce::PathStrokeType (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (arcBlue.withAlpha (0.20f));
        g.strokePath (value, juce::PathStrokeType (10.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (arcBlue.brighter (0.15f));
        g.strokePath (value, juce::PathStrokeType (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // dark glossy bezel
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34363c), c.x, c.y - bezelR, juce::Colour (0xff060708), c.x, c.y + bezelR, false));
        g.fillEllipse (c.x - bezelR, c.y - bezelR, bezelR * 2.f, bezelR * 2.f);
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - bezelR, c.y - bezelR, bezelR * 2.f, bezelR * 2.f, 1.2f);

        // brushed-metal cap
        juce::Path capPath;
        capPath.addEllipse (c.x - capR, c.y - capR, capR * 2.f, capR * 2.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6d727c), c.x - capR * 0.4f, c.y - capR * 0.6f,
                                                 juce::Colour (0xff1f2227), c.x + capR, c.y + capR, true));
        g.fillPath (capPath);
        g.saveState();
        g.reduceClipRegion (capPath);
        for (int i = 0; i < 120; ++i)   // radial brushing
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 120.f;
            const float al = 0.03f + 0.07f * hash01 (i);
            g.setColour ((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha (al));
            g.drawLine (c.x, c.y, c.x + std::sin (a) * capR, c.y - std::cos (a) * capR, 0.8f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.13f));   // anisotropic highlights
        for (float base : { -0.95f, juce::MathConstants<float>::pi - 0.95f })
        {
            juce::Path wedge;
            wedge.addPieSegment (c.x - capR, c.y - capR, capR * 2.f, capR * 2.f, base, base + 0.55f, 0.f);
            g.fillPath (wedge);
        }
        g.restoreState();
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.f, capR * 2.f, 1.3f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawEllipse (c.x - capR + 1.2f, c.y - capR + 1.2f, capR * 2.f - 2.4f, capR * 2.f - 2.4f, 0.8f);

        // pointer
        juce::Path ptr;
        ptr.addRoundedRectangle (-1.7f, -capR * 0.93f, 3.4f, capR * 0.38f, 1.5f);
        auto tf = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillPath (ptr, tf.translated (0.f, 1.f));
        g.setColour (juce::Colour (0xffdadde3));
        g.fillPath (ptr, tf);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        using namespace PeakyColours;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0a0b0d), 0, b.getY(), juce::Colour (0xff1b1d22), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 4.f);
        auto fill = b.withRight (juce::jlimit (b.getX(), b.getRight(), sliderPos));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff58c6de), 0, b.getY(), juce::Colour (0xff23829c), 0, b.getBottom(), false));
        g.fillRoundedRectangle (fill, 4.f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine ((int) b.getY() + 2, fill.getX() + 3.f, fill.getRight() - 3.f);
        g.setColour (juce::Colour (0xff3d4048));
        g.drawRoundedRectangle (b, 4.f, 1.f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
        g.drawText (s.getTextFromValue (s.getValue()), b.translated (0.f, 1.f), juce::Justification::centred);
        g.setColour (juce::Colours::white);
        g.drawText (s.getTextFromValue (s.getValue()), b, juce::Justification::centred);
    }

    void drawLabel (juce::Graphics& g, juce::Label& l) override
    {
        using namespace PeakyColours;
        if (dynamic_cast<juce::Slider*> (l.getParentComponent()) == nullptr)
        {
            LookAndFeel_V4::drawLabel (g, l);
            return;
        }
        auto b = l.getLocalBounds().toFloat().reduced (1.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d1c23), 0, b.getY(), juce::Colour (0xff081318), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 4.f);
        g.setColour (edge);
        g.drawRoundedRectangle (b, 4.f, 1.f);
        if (! l.isBeingEdited())
        {
            g.setColour (juce::Colour (0xffa6f3fb));
            g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
            g.drawText (l.getText(), b, juce::Justification::centred);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& bt, const juce::Colour&, bool over, bool down) override
    {
        using namespace PeakyColours;
        auto b = bt.getLocalBounds().toFloat().reduced (1.5f);
        const bool on = bt.getToggleState();
        juce::Colour top (0xff3b3d45), bot (0xff222328);
        if (over) { top = top.brighter (0.12f); bot = bot.brighter (0.12f); }
        if (down) { top = top.darker (0.25f);   bot = bot.darker (0.25f); }
        if (on) { g.setColour (teal.withAlpha (0.22f)); g.drawRoundedRectangle (b, 4.f, 3.f); }
        g.setGradientFill (juce::ColourGradient (top, 0, b.getY(), bot, 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 4.f);
        g.setColour (juce::Colours::white.withAlpha (0.09f));
        g.drawHorizontalLine ((int) b.getY() + 1, b.getX() + 3.f, b.getRight() - 3.f);
        g.setColour (on ? teal : juce::Colour (0xff5c5f68));
        g.drawRoundedRectangle (b, 4.f, 1.f);
        if (! bt.isEnabled()) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (b, 4.f); }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (12.f, juce::Font::bold)); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto b = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2e34), 0, b.getY(), juce::Colour (0xff1a1b1f), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 4.f);
        g.setColour (juce::Colour (0xff55585f));
        g.drawRoundedRectangle (b, 4.f, 1.f);
        const float cx = (float) w - 12.f, cy = (float) h * 0.5f;
        juce::Path up, dn;
        up.addTriangle (cx - 3.5f, cy - 2.f, cx + 3.5f, cy - 2.f, cx, cy - 6.f);
        dn.addTriangle (cx - 3.5f, cy + 2.f, cx + 3.5f, cy + 2.f, cx, cy + 6.f);
        g.setColour (juce::Colour (0xffbfc3ca));
        g.fillPath (up);
        g.fillPath (dn);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 1, box.getWidth() - 28, box.getHeight() - 2);
        label.setFont (juce::Font (juce::FontOptions (13.f, juce::Font::bold)));
    }
};
