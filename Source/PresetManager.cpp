#include "PresetManager.h"

namespace
{
struct FactoryPreset
{
    const char* name;
    float thr, tgt, smooth, maxGr, post;
    int lookaheadOn, lookaheadIdx, detect;
    float mix;
};
const FactoryPreset kFactory[] = {
#include "FactoryPresets.inc"
};
constexpr int kNumFactory = (int) (sizeof (kFactory) / sizeof (kFactory[0]));
const char* kExt = ".peakypreset";
const char* kSavedIds[] = { IDs::threshold, IDs::target, IDs::smoothing, IDs::maxgr, IDs::postgain,
                            IDs::lookahead, IDs::lookaheadTime, IDs::detect, IDs::mix };
}

int PresetManager::getNumFactoryPresets() { return kNumFactory; }
juce::String PresetManager::getFactoryPresetName (int i) { return juce::String (kFactory[juce::jlimit (0, kNumFactory - 1, i)].name); }

juce::File PresetManager::getUserDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Aron H").getChildFile ("Peaky").getChildFile ("Presets");
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (auto& f : getUserDirectory().findChildFiles (juce::File::findFiles, false, juce::String ("*") + kExt))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

void PresetManager::setParam (const char* id, float v)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

void PresetManager::loadFactory (int i)
{
    if (i < 0 || i >= kNumFactory) return;
    const auto& f = kFactory[i];
    setParam (IDs::threshold, f.thr);
    setParam (IDs::target, f.tgt);
    setParam (IDs::smoothing, f.smooth);
    setParam (IDs::maxgr, f.maxGr);
    setParam (IDs::postgain, f.post);
    setParam (IDs::lookahead, (float) f.lookaheadOn);
    setParam (IDs::lookaheadTime, (float) f.lookaheadIdx);
    setParam (IDs::detect, (float) f.detect);
    setParam (IDs::mix, f.mix);
    setCurrentName (f.name);
}

bool PresetManager::applyXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("PeakyPreset")) return false;
    for (auto* e : xml.getChildWithTagNameIterator ("Param"))
        for (auto* id : kSavedIds)
            if (e->getStringAttribute ("id") == id)
                setParam (id, (float) e->getDoubleAttribute ("value"));
    setCurrentName (xml.getStringAttribute ("name", "Preset"));
    return true;
}

bool PresetManager::loadUser (const juce::String& name)
{
    auto f = getUserDirectory().getChildFile (juce::File::createLegalFileName (name) + kExt);
    if (auto xml = juce::parseXML (f)) return applyXml (*xml);
    return false;
}

bool PresetManager::loadFromFile (const juce::File& f)
{
    if (auto xml = juce::parseXML (f)) return applyXml (*xml);
    return false;
}

juce::XmlElement PresetManager::createXml (const juce::String& name) const
{
    juce::XmlElement xml ("PeakyPreset");
    xml.setAttribute ("name", name);
    xml.setAttribute ("version", 1);
    for (auto* id : kSavedIds)
    {
        auto* e = xml.createNewChildElement ("Param");
        e->setAttribute ("id", id);
        e->setAttribute ("value", (double) apvts.getRawParameterValue (id)->load());
    }
    return xml;
}

bool PresetManager::saveUser (const juce::String& rawName)
{
    auto name = rawName.trim();
    if (name.isEmpty()) return false;
    for (int i = 0; i < kNumFactory; ++i)
        if (name.equalsIgnoreCase (kFactory[i].name)) return false;
    auto dir = getUserDirectory();
    if (! dir.createDirectory()) return false;
    if (! saveToFile (dir.getChildFile (juce::File::createLegalFileName (name) + kExt), name)) return false;
    setCurrentName (name);
    return true;
}

bool PresetManager::saveToFile (const juce::File& file, const juce::String& name)
{
    return createXml (name).writeTo (file);
}

bool PresetManager::deleteUser (const juce::String& name)
{
    return getUserDirectory().getChildFile (juce::File::createLegalFileName (name) + kExt).deleteFile();
}

juce::String PresetManager::getCurrentName() const
{
    return apvts.state.getProperty ("presetName", "Natural Smoothing").toString();
}
void PresetManager::setCurrentName (const juce::String& n) { apvts.state.setProperty ("presetName", n, nullptr); }
bool PresetManager::isCurrentUser() const { return getUserPresetNames().contains (getCurrentName()); }
