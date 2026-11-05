#pragma once
#include "Parameters.h"
#include "PeakyDSP.h"
#include "PresetManager.h"
#include <juce_dsp/juce_dsp.h>

class PeakyAudioProcessor : public juce::AudioProcessor
{
public:
    PeakyAudioProcessor();
    ~PeakyAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter (IDs::bypass); }

    juce::AudioProcessorValueTreeState apvts;   // must be declared before presets
    PresetManager presets;
    peaky::PeakySmoother dsp;      // base-rate processor
    peaky::PeakySmoother dspOS;    // runs at 2x sample rate when oversampling is on
    const peaky::PeakySmoother& getActiveDsp() const noexcept { return osActive.load() ? dspOS : dsp; }

private:
    peaky::Params makeParams() const;
    int computeLatency (const peaky::Params&, bool os) const;
    void processOversampled (juce::AudioBuffer<float>&, const peaky::Params&);
    std::atomic<float> *thr, *tgt, *smooth, *maxgr, *post, *laOn, *laTime, *detect, *mix, *byp, *osParam;
    juce::dsp::Oversampling<float> oversampler;
    std::atomic<bool> osActive { false };
    bool lastOs = false;
    int maxBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakyAudioProcessor)
};
