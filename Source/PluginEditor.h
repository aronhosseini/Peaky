#pragma once
#include "PluginProcessor.h"
#include "PeakyLookAndFeel.h"
#include "ScopeDisplay.h"

// ON / OFF pill + glowing green status light. Lit = plugin is processing; click to bypass.
class PowerButton : public juce::Button
{
public:
    PowerButton() : juce::Button ("power") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.5f);
        const bool active = getToggleState();
        auto left = b.removeFromLeft (b.getWidth() * 0.5f - 1.f);
        g.setGradientFill (juce::ColourGradient (active ? juce::Colour (0xff12301f) : juce::Colour (0xff26221d), 0, left.getY(),
                                                 active ? juce::Colour (0xff0a1a11) : juce::Colour (0xff14110f), 0, left.getBottom(), false));
        g.fillRoundedRectangle (left, 4.f);
        g.setColour (active ? juce::Colour (0xff215c37) : juce::Colour (0xff4a4137));
        g.drawRoundedRectangle (left, 4.f, 1.f);
        g.setColour (active ? juce::Colour (0xff3fe074) : juce::Colour (0xffffb04a));
        g.setFont (juce::Font (juce::FontOptions (12.f, juce::Font::bold)));
        g.drawText (active ? "ON" : "BYP", left, juce::Justification::centred);
        b.removeFromLeft (2.f);
        const juce::Colour led = active ? juce::Colour (0xff35d56b) : juce::Colour (0xff2b1c1c);
        if (active) { g.setColour (led.withAlpha (0.30f)); g.drawRoundedRectangle (b, 5.f, 3.f); }
        g.setGradientFill (juce::ColourGradient (led.brighter (over ? 0.25f : 0.1f), 0, b.getY(), led.darker (0.45f), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 5.f);
        g.setColour (juce::Colours::white.withAlpha (active ? 0.22f : 0.05f));
        g.drawHorizontalLine ((int) b.getY() + 2, b.getX() + 4.f, b.getRight() - 4.f);
        g.setColour (juce::Colour (0xff050607));
        g.drawRoundedRectangle (b, 5.f, 1.f);
    }
};

class PeakyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PeakyAudioProcessorEditor (PeakyAudioProcessor&);
    ~PeakyAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void rebuildPresetMenu();
    void selectCurrentPreset();
    void stepPreset (int dir);
    void showGearMenu();
    void promptSavePreset();

    PeakyAudioProcessor& proc;
    PeakyLookAndFeel laf;

    ScopeDisplay scope;
    GainReductionMeter meter;

    juce::Slider knobs[5], mixSlider;
    std::unique_ptr<SliderAtt> knobAtt[5], mixAtt;

    juce::HyperlinkButton link { "ARON H", juce::URL ("https://www.instagram.com/aronh.official") };
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, gearBtn { "..." };

    juce::TextButton laOnBtn { "ON" }, laTimeBtn, rmsBtn { "RMS" }, peakBtn { "PEAK" };
    PowerButton powerBtn;
    juce::TextButton osBtn { "2x" };
    std::unique_ptr<ButtonAtt> laOnAtt, osAtt;
    std::unique_ptr<juce::ParameterAttachment> laTimeAtt, detectAtt, bypassAtt;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakyAudioProcessorEditor)
};
