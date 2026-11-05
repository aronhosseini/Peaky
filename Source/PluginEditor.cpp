#include "PluginEditor.h"

namespace
{
constexpr int kW = 660, kH = 460;
constexpr float kUiScale = 1.2f;   // fixed window size = 20% bigger than the 660x460 design
constexpr float kKnobCell = (kW - 20) / 5.f;
const char* kKnobIds[5]   = { IDs::threshold, IDs::target, IDs::smoothing, IDs::maxgr, IDs::postgain };
const char* kKnobNames[5] = { "THRESHOLD", "TARGET LEVEL", "SMOOTHING", "MAX GAIN RED", "POST GAIN" };
const char* kKnobMin[5]   = { "-48dB", "-36dB", "5ms", "0dB", "-12dB" };
const char* kKnobMax[5]   = { "0dB", "-3dB", "50ms", "-18dB", "+12dB" };
}

PeakyAudioProcessorEditor::PeakyAudioProcessorEditor (PeakyAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), scope (p), meter (p)
{
    setLookAndFeel (&laf);
    setResizable (false, false);   // fixed size: exact scale keeps every text/knob in its place
    setSize (juce::roundToInt ((float) kW * kUiScale), juce::roundToInt ((float) kH * kUiScale));   // 792 x 552

    addAndMakeVisible (scope);
    addAndMakeVisible (meter);

    for (int i = 0; i < 5; ++i)
    {
        auto& s = knobs[i];
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 76, 22);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        s.setMouseDragSensitivity (200);
        addAndMakeVisible (s);
        knobAtt[i] = std::make_unique<SliderAtt> (proc.apvts, kKnobIds[i], s);
        auto* par = proc.apvts.getParameter (kKnobIds[i]);
        s.setDoubleClickReturnValue (true, par->convertFrom0to1 (par->getDefaultValue()));
    }

    mixSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (mixSlider);
    mixAtt = std::make_unique<SliderAtt> (proc.apvts, IDs::mix, mixSlider);
    mixSlider.setDoubleClickReturnValue (true, 100.0);

    // ---- header -------------------------------------------------------------
    link.setFont (juce::Font (juce::FontOptions (15.f, juce::Font::bold)), false, juce::Justification::centredLeft);
    link.setTooltip ("Aron H on Instagram");
    addAndMakeVisible (link);

    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1000)    proc.presets.loadUser (presetBox.getText());
        else if (id >= 1)  proc.presets.loadFactory (id - 1);
    };
    for (auto* b : { &prevBtn, &nextBtn, &gearBtn }) addAndMakeVisible (b);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    gearBtn.onClick = [this] { showGearMenu(); };
    rebuildPresetMenu();

    // ---- bottom bar -----------------------------------------------------------
    for (auto* b : { &laOnBtn, &laTimeBtn, &rmsBtn, &peakBtn, &osBtn }) addAndMakeVisible (b);
    osBtn.setClickingTogglesState (true);
    osAtt = std::make_unique<ButtonAtt> (proc.apvts, IDs::oversample, osBtn);
    osBtn.setTooltip ("2x oversampling (more accurate peak detection, adds latency)");
    addAndMakeVisible (powerBtn);

    laOnBtn.setClickingTogglesState (true);
    laOnAtt = std::make_unique<ButtonAtt> (proc.apvts, IDs::lookahead, laOnBtn);
    laOnBtn.onStateChange = [this] { laTimeBtn.setEnabled (laOnBtn.getToggleState()); };
    laTimeBtn.setEnabled (laOnBtn.getToggleState());

    laTimeAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (IDs::lookaheadTime), [this] (float v)
    {
        const int i = juce::jlimit (0, kNumLookaheadChoices - 1, (int) std::lround (v));
        laTimeBtn.setButtonText (juce::String ((int) kLookaheadChoicesMs[i]) + "ms");
    });
    laTimeAtt->sendInitialUpdate();
    laTimeBtn.onClick = [this]
    {
        const int cur = (int) std::lround (proc.apvts.getRawParameterValue (IDs::lookaheadTime)->load());
        laTimeAtt->setValueAsCompleteGesture ((float) ((cur + 1) % kNumLookaheadChoices));
    };

    detectAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (IDs::detect), [this] (float v)
    {
        rmsBtn.setToggleState (v < 0.5f, juce::dontSendNotification);
        peakBtn.setToggleState (v >= 0.5f, juce::dontSendNotification);
    });
    detectAtt->sendInitialUpdate();
    rmsBtn.onClick  = [this] { detectAtt->setValueAsCompleteGesture (0.f); };
    peakBtn.onClick = [this] { detectAtt->setValueAsCompleteGesture (1.f); };

    bypassAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (IDs::bypass), [this] (float v)
    {
        powerBtn.setToggleState (v < 0.5f, juce::dontSendNotification);   // lit = active
    });
    bypassAtt->sendInitialUpdate();
    powerBtn.onClick = [this] { bypassAtt->setValueAsCompleteGesture (powerBtn.getToggleState() ? 0.f : 1.f); };
}

PeakyAudioProcessorEditor::~PeakyAudioProcessorEditor() { setLookAndFeel (nullptr); }

//==============================================================================
void PeakyAudioProcessorEditor::rebuildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < PresetManager::getNumFactoryPresets(); ++i)
        presetBox.addItem (PresetManager::getFactoryPresetName (i), i + 1);
    auto user = proc.presets.getUserPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User");
        for (int j = 0; j < user.size(); ++j) presetBox.addItem (user[j], 1000 + j);
    }
    selectCurrentPreset();
}

void PeakyAudioProcessorEditor::selectCurrentPreset()
{
    const auto name = proc.presets.getCurrentName();
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == name) { presetBox.setSelectedItemIndex (i, juce::dontSendNotification); return; }
    presetBox.setText (name, juce::dontSendNotification);
}

void PeakyAudioProcessorEditor::stepPreset (int dir)
{
    const int n = presetBox.getNumItems();
    if (n == 0) return;
    int idx = presetBox.getSelectedItemIndex();
    idx = idx < 0 ? 0 : (idx + dir + n) % n;
    presetBox.setSelectedItemIndex (idx, juce::sendNotificationSync);
}

void PeakyAudioProcessorEditor::showGearMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Save Preset...");
    m.addItem (2, "Export Preset to File...");
    m.addItem (3, "Load Preset from File...");
    m.addItem (4, "Delete Current User Preset", proc.presets.isCurrentUser());
    m.addItem (5, "Open User Presets Folder");
    m.addSeparator();
    m.addItem (6, "About Peaky...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (gearBtn), [this, safe = juce::Component::SafePointer<PeakyAudioProcessorEditor> (this)] (int r)
    {
        if (safe == nullptr) return;
        switch (r)
        {
            case 1: promptSavePreset(); break;
            case 2:
            {
                chooser = std::make_unique<juce::FileChooser> ("Export preset", proc.presets.getUserDirectory().getChildFile (proc.presets.getCurrentName() + ".peakypreset"), "*.peakypreset");
                chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                                      [this] (const juce::FileChooser& fc)
                                      { if (fc.getResult() != juce::File()) proc.presets.saveToFile (fc.getResult().withFileExtension ("peakypreset"), fc.getResult().getFileNameWithoutExtension()); });
                break;
            }
            case 3:
            {
                chooser = std::make_unique<juce::FileChooser> ("Load preset", proc.presets.getUserDirectory(), "*.peakypreset");
                chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                      [this] (const juce::FileChooser& fc)
                                      { if (fc.getResult().existsAsFile()) { proc.presets.loadFromFile (fc.getResult()); selectCurrentPreset(); } });
                break;
            }
            case 4:
                if (proc.presets.deleteUser (proc.presets.getCurrentName())) { proc.presets.loadFactory (0); rebuildPresetMenu(); }
                break;
            case 5:
                proc.presets.getUserDirectory().createDirectory();
                proc.presets.getUserDirectory().revealToUser();
                break;
            case 6:
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "About Peaky",
                    juce::String ("Peaky v") + JucePlugin_VersionString + "\nAutomatic pre-compression vocal peak smoother\n\nDeveloped by Aron H\ninstagram.com/aronh.official");
                break;
            default: break;
        }
    });
}

void PeakyAudioProcessorEditor::promptSavePreset()
{
    auto* aw = new juce::AlertWindow ("Save Preset", "Name your preset:", juce::MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", proc.presets.isCurrentUser() ? proc.presets.getCurrentName() : juce::String ("My Preset"));
    aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw, safe = juce::Component::SafePointer<PeakyAudioProcessorEditor> (this)] (int r)
    {
        if (r != 1 || safe == nullptr) return;
        if (proc.presets.saveUser (aw->getTextEditorContents ("name"))) rebuildPresetMenu();
        else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Preset not saved",
                 "Please choose a different name (factory preset names are reserved).");
    }), true);
}

//==============================================================================
void PeakyAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace PeakyColours;
    g.addTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kW));

    // charcoal body with fine brushed-metal grain
    g.setGradientFill (juce::ColourGradient (bgTop, 0, 0, bgBot, 0, (float) kH, false));
    g.fillAll();
    for (int yy = 0; yy < kH; yy += 2)
    {
        g.setColour (juce::Colours::white.withAlpha (0.006f + 0.02f * hash01 (yy)));
        g.fillRect (0, yy, kW, 1);
    }

    // header
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35363c), 0, 0, juce::Colour (0xff1c1d21), 0, 35.f, false));
    g.fillRect (0, 0, kW, 35);
    for (int yy = 0; yy < 35; ++yy)
    {
        g.setColour (juce::Colours::white.withAlpha (0.01f + 0.035f * hash01 (yy + 77)));
        g.fillRect (0, yy, kW, 1);
    }
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawHorizontalLine (35, 0.f, (float) kW);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (36, 0.f, (float) kW);

    g.setColour (juce::Colour (0xff8e929b));
    g.setFont (juce::Font (juce::FontOptions (10.f)));
    g.drawText ("Developed by", 12, 7, 66, 20, juce::Justification::centredLeft);

    // embossed PEAKY
    g.setFont (juce::Font (juce::FontOptions (24.f, juce::Font::bold).withKerningFactor (0.14f)));
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawText ("PEAKY", 0, 4, kW, 28, juce::Justification::centred);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff6f7f9), 0, 6.f, juce::Colour (0xff8a8f99), 0, 30.f, false));
    g.drawText ("PEAKY", 0, 2, kW, 28, juce::Justification::centred);

    // GR meter caption
    g.setColour (juce::Colour (0xffb4b8c0));
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawText ("GAIN REDUCTION", 574, 40, 82, 12, juce::Justification::centred);

    // knob captions and range labels
    for (int i = 0; i < 5; ++i)
    {
        const float cx = 10.f + kKnobCell * ((float) i + 0.5f);
        g.setColour (juce::Colour (0xffd2d5db));
        g.setFont (juce::Font (juce::FontOptions (13.f, juce::Font::bold)));
        g.drawText (kKnobNames[i], (int) (cx - 62.f), 388, 124, 14, juce::Justification::centred);
        g.setColour (dim);
        g.setFont (juce::Font (juce::FontOptions (10.5f)));
        g.drawText (kKnobMin[i], (int) (cx - 62.f), 372, 46, 12, juce::Justification::centredLeft);
        g.drawText (kKnobMax[i], (int) (cx + 16.f), 372, 46, 12, juce::Justification::centredRight);
    }

    // bottom bar
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff17181c), 0, 404.f, juce::Colour (0xff0d0e10), 0, (float) kH, false));
    g.fillRect (0, 404, kW, kH - 404);
    g.setColour (juce::Colours::black);
    g.drawHorizontalLine (404, 0.f, (float) kW);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine (405, 0.f, (float) kW);
    for (int sx : { 148, 278, 426, 531 }) g.drawVerticalLine (sx, 412.f, 452.f);

    g.setColour (juce::Colour (0xffb4b8c0));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("LOOKAHEAD", 13, 408, 140, 13, juce::Justification::centred);
    g.drawText ("DETECT MODE", 143, 408, 140, 13, juce::Justification::centred);
    g.drawText ("MIX", 296, 408, 120, 13, juce::Justification::centred);
    g.drawText ("OVERSAMPLE", 435, 408, 90, 13, juce::Justification::centred);
    g.drawText ("BYPASS", 514, 408, 140, 13, juce::Justification::centred);
    g.setColour (dim);
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.drawText ("DRY/WET", 296, 446, 120, 12, juce::Justification::centred);
}

void PeakyAudioProcessorEditor::resized()
{
    link.setBounds (76, 7, 100, 20);
    prevBtn.setBounds (kW - 248, 7, 20, 20);
    nextBtn.setBounds (kW - 226, 7, 20, 20);
    presetBox.setBounds (kW - 202, 6, 170, 22);
    gearBtn.setBounds (kW - 28, 7, 20, 20);

    scope.setBounds (8, 40, 566, 220);
    meter.setBounds (580, 54, 72, 206);

    for (int i = 0; i < 5; ++i)
    {
        const int cx = (int) (10.f + kKnobCell * ((float) i + 0.5f));
        knobs[i].setBounds (cx - 56, 266, 112, 112);
    }

    laOnBtn.setBounds (36, 424, 46, 22);
    laTimeBtn.setBounds (84, 424, 46, 22);
    rmsBtn.setBounds (166, 424, 46, 22);
    peakBtn.setBounds (214, 424, 46, 22);
    mixSlider.setBounds (296, 424, 120, 22);
    osBtn.setBounds (452, 424, 56, 22);
    powerBtn.setBounds (544, 424, 80, 22);

    // the layout is designed at 660x460; scale every child with the window (aspect ratio is locked)
    const auto t = juce::AffineTransform::scale ((float) getWidth() / (float) kW);
    for (auto* c : getChildren()) c->setTransform (t);
}
