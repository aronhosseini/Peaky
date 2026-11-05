#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace IDs
{
inline constexpr const char* threshold     = "threshold";
inline constexpr const char* target        = "target";
inline constexpr const char* smoothing     = "smoothing";
inline constexpr const char* maxgr         = "maxgr";      // stored as positive dB magnitude, shown as "-8dB"
inline constexpr const char* postgain      = "postgain";
inline constexpr const char* lookahead     = "lookahead";
inline constexpr const char* lookaheadTime = "lookaheadtime";
inline constexpr const char* detect        = "detect";     // 0 = RMS, 1 = PEAK
inline constexpr const char* mix           = "mix";        // percent
inline constexpr const char* bypass        = "bypass";
inline constexpr const char* oversample    = "oversample";  // 2x oversampling
}

inline constexpr float kLookaheadChoicesMs[] = { 5.f, 10.f, 15.f, 20.f, 30.f, 50.f };
inline constexpr int   kNumLookaheadChoices  = 6;

namespace ParamFmt
{
inline juce::String num (float v, bool plus = false)
{
    if (std::abs (v) < 0.05f) return "0";
    auto s = juce::String (v, 1);
    if (s.endsWith (".0")) s = s.dropLastCharacters (2);
    if (plus && v > 0.f) s = "+" + s;
    return s;
}
inline float parse (const juce::String& s) { return s.retainCharacters ("0123456789.-+").getFloatValue(); }
}

inline std::unique_ptr<juce::AudioParameterFloat> makeFloatParam (const char* id, const char* name,
                                                                  float lo, float hi, float step, float def,
                                                                  std::function<juce::String (float)> toText,
                                                                  std::function<float (const juce::String&)> fromText)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, step), def,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([toText] (float v, int) { return toText (v); })
            .withValueFromStringFunction ([fromText] (const juce::String& s) { return fromText (s); }));
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace ParamFmt;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (makeFloatParam (IDs::threshold, "Threshold", -48.f, 0.f, 0.1f, -10.f,
                           [] (float v) { return num (v) + "dB"; }, [] (const juce::String& s) { return parse (s); }));
    l.add (makeFloatParam (IDs::target, "Target Level", -36.f, -3.f, 0.1f, -12.f,
                           [] (float v) { return num (v) + "dB"; }, [] (const juce::String& s) { return parse (s); }));
    l.add (makeFloatParam (IDs::smoothing, "Smoothing", 5.f, 50.f, 0.5f, 15.f,
                           [] (float v) { return num (v) + "ms"; }, [] (const juce::String& s) { return parse (s); }));
    l.add (makeFloatParam (IDs::maxgr, "Max Gain Reduction", 0.f, 18.f, 0.1f, 8.f,
                           [] (float v) { return v < 0.05f ? juce::String ("0dB") : "-" + num (v) + "dB"; },
                           [] (const juce::String& s) { return std::abs (parse (s)); }));
    l.add (makeFloatParam (IDs::postgain, "Post Gain", -12.f, 12.f, 0.1f, 2.f,
                           [] (float v) { return num (v, true) + "dB"; }, [] (const juce::String& s) { return parse (s); }));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { IDs::lookahead, 1 }, "Lookahead", true));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { IDs::lookaheadTime, 1 }, "Lookahead Time",
                                                         juce::StringArray { "5 ms", "10 ms", "15 ms", "20 ms", "30 ms", "50 ms" }, 2));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { IDs::detect, 1 }, "Detect Mode",
                                                         juce::StringArray { "RMS", "PEAK" }, 0));
    l.add (makeFloatParam (IDs::mix, "Mix", 0.f, 100.f, 1.f, 100.f,
                           [] (float v) { return juce::String (juce::roundToInt (v)) + "%"; },
                           [] (const juce::String& s) { return parse (s); }));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { IDs::bypass, 1 }, "Bypass", false));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { IDs::oversample, 1 }, "Oversampling 2x", false));
    return l;
}
