#include "PluginProcessor.h"
#include "PluginEditor.h"

PeakyAudioProcessor::PeakyAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout()),
      presets (apvts),
      oversampler (2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true)
{
    thr     = apvts.getRawParameterValue (IDs::threshold);
    tgt     = apvts.getRawParameterValue (IDs::target);
    smooth  = apvts.getRawParameterValue (IDs::smoothing);
    maxgr   = apvts.getRawParameterValue (IDs::maxgr);
    post    = apvts.getRawParameterValue (IDs::postgain);
    laOn    = apvts.getRawParameterValue (IDs::lookahead);
    laTime  = apvts.getRawParameterValue (IDs::lookaheadTime);
    detect  = apvts.getRawParameterValue (IDs::detect);
    mix     = apvts.getRawParameterValue (IDs::mix);
    byp     = apvts.getRawParameterValue (IDs::bypass);
    osParam = apvts.getRawParameterValue (IDs::oversample);
    presets.setCurrentName ("Natural Smoothing");   // parameter defaults == factory preset 1
}

peaky::Params PeakyAudioProcessor::makeParams() const
{
    peaky::Params p;
    p.thresholdDb = thr->load();
    p.targetDb    = tgt->load();
    p.smoothingMs = smooth->load();
    p.maxGrDb     = maxgr->load();
    p.postGainDb  = post->load();
    p.mix         = mix->load() * 0.01f;
    p.lookaheadOn = laOn->load() > 0.5f;
    p.lookaheadMs = kLookaheadChoicesMs[juce::jlimit (0, kNumLookaheadChoices - 1, (int) std::lround (laTime->load()))];
    p.peakMode    = detect->load() > 0.5f;
    p.bypass      = byp->load() > 0.5f;
    return p;
}

int PeakyAudioProcessor::computeLatency (const peaky::Params& p, bool os) const
{
    if (! os) return dsp.computeDelaySamples (p);
    const int d2 = dspOS.computeDelaySamples (p);               // lookahead counted at 2x rate
    return (d2 + 1) / 2 + (int) std::lround (oversampler.getLatencyInSamples());
}

void PeakyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlock = juce::jmax (1, samplesPerBlock);
    const int ch = juce::jmax (2, getTotalNumInputChannels());
    dsp.prepare (sampleRate, ch);
    dspOS.prepare (sampleRate * 2.0, ch);
    oversampler.initProcessing ((size_t) maxBlock);
    oversampler.reset();
    lastOs = osParam->load() > 0.5f;
    osActive.store (lastOs);
    setLatencySamples (computeLatency (makeParams(), lastOs));
}

void PeakyAudioProcessor::processOversampled (juce::AudioBuffer<float>& buffer, const peaky::Params& p)
{
    const int nCh = juce::jmin (2, buffer.getNumChannels()), total = buffer.getNumSamples();
    for (int start = 0; start < total; start += maxBlock)
    {
        const int len = juce::jmin (maxBlock, total - start);
        juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), nCh, start, len);
        juce::dsp::AudioBlock<float> block (sub);
        auto up = oversampler.processSamplesUp (block);
        float* ptrs[2] = { up.getChannelPointer (0), nCh > 1 ? up.getChannelPointer (1) : nullptr };
        juce::AudioBuffer<float> ub (ptrs, nCh, (int) up.getNumSamples());
        dspOS.process (ub, p);
        oversampler.processSamplesDown (block);
    }
}

bool PeakyAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void PeakyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    const auto p = makeParams();
    const bool wantOs = osParam->load() > 0.5f;
    if (wantOs != lastOs)             // switching: flush all state (brief glitch, latency changes)
    {
        lastOs = wantOs;
        dsp.reset(); dspOS.reset(); oversampler.reset();
        osActive.store (wantOs);
    }
    if (wantOs) processOversampled (buffer, p);
    else        dsp.process (buffer, p);

    const int lat = computeLatency (p, wantOs);
    if (lat != getLatencySamples()) setLatencySamples (lat);
}

juce::AudioProcessorEditor* PeakyAudioProcessor::createEditor() { return new PeakyAudioProcessorEditor (*this); }

void PeakyAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void PeakyAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PeakyAudioProcessor(); }
