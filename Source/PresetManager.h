#pragma once
#include "Parameters.h"

// 20 factory presets (compiled in, generated from tools/gen_presets.py) + user presets
// stored as .peakypreset XML files in  <Documents>/Aron H/Peaky/Presets
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

    static int getNumFactoryPresets();
    static juce::String getFactoryPresetName (int index);

    juce::File getUserDirectory() const;
    juce::StringArray getUserPresetNames() const;

    void loadFactory (int index);
    bool loadUser (const juce::String& name);
    bool loadFromFile (const juce::File& file);
    bool saveUser (const juce::String& name);               // false if name invalid / clashes with factory
    bool saveToFile (const juce::File& file, const juce::String& name);
    bool deleteUser (const juce::String& name);

    juce::String getCurrentName() const;
    void setCurrentName (const juce::String& n);
    bool isCurrentUser() const;

private:
    juce::XmlElement createXml (const juce::String& name) const;
    bool applyXml (const juce::XmlElement& xml);
    void setParam (const char* id, float plainValue);

    juce::AudioProcessorValueTreeState& apvts;
};
