// Peaky - Automatic pre-compression vocal peak smoother
// Developed by Aron H  (https://www.instagram.com/aronh.official)
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

namespace peaky
{
struct Params
{
    float thresholdDb = -10.f;   // detector level above which smoothing starts
    float targetDb    = -12.f;   // level that peaks are pulled toward
    float smoothingMs = 15.f;    // gain ramp time (clamped to the lookahead time)
    float maxGrDb     = 8.f;     // maximum attenuation, positive magnitude
    float postGainDb  = 2.f;
    float mix         = 1.f;     // 0..1
    bool  lookaheadOn = true;
    float lookaheadMs = 15.f;
    bool  peakMode    = false;   // false = RMS detector, true = peak detector
    bool  bypass      = false;
};

//==============================================================================
// Sliding-window minimum, O(1) amortised (monotonic deque).
class SlidingMin
{
public:
    void prepare (int maxWindow)
    {
        size = (size_t) juce::nextPowerOfTwo (maxWindow + 2);
        mask = size - 1;
        vals.assign (size, 0.f);
        idxs.assign (size, 0);
        reset();
    }
    void reset() { head = tail = 0; counter = 0; }

    float push (float v, int window)
    {
        while (tail != head && vals[(size_t) ((tail - 1) & (int64_t) mask)] >= v) --tail;
        vals[(size_t) (tail & (int64_t) mask)] = v;
        idxs[(size_t) (tail & (int64_t) mask)] = counter;
        ++tail;
        while (idxs[(size_t) (head & (int64_t) mask)] <= counter - window) ++head;
        ++counter;
        return vals[(size_t) (head & (int64_t) mask)];
    }

private:
    std::vector<float> vals;
    std::vector<int64_t> idxs;
    size_t size = 0, mask = 0;
    int64_t head = 0, tail = 0, counter = 0;
};

//==============================================================================
class MovingAverage
{
public:
    void prepare (int maxLen)
    {
        const size_t sz = (size_t) juce::nextPowerOfTwo (maxLen + 2);
        buf.assign (sz, 0.f);
        mask = sz - 1;
        len = 1;
        fill (0.f);
    }
    void reset() { fill (0.f); }
    void setLength (int n)
    {
        n = std::max (1, n);
        if (n != len) { len = n; fill (last); }
    }
    float process (float x)
    {
        sum += (double) x - (double) buf[(pos - (size_t) len) & mask];
        buf[pos & mask] = x;
        ++pos;
        last = (float) (sum / (double) len);
        return last;
    }

private:
    void fill (float v)
    {
        std::fill (buf.begin(), buf.end(), v);
        sum = (double) v * len;
        pos = 0;
        last = v;
    }
    std::vector<float> buf;
    size_t mask = 0, pos = 0;
    int len = 1;
    double sum = 0.0;
    float last = 0.f;
};

//==============================================================================
class PeakySmoother
{
public:
    static constexpr int kHistoryColumns = 512;
    static constexpr int kMaxChannels = 16;

    void prepare (double sampleRate, int maxChannels)
    {
        sr = sampleRate;
        numCh = juce::jlimit (1, kMaxChannels, maxChannels);
        maxDelay = (int) std::ceil (0.050 * sr) + 4;           // 50 ms max lookahead
        slidingMin.prepare (maxDelay + 2);
        ma1.prepare (maxDelay + 2);
        ma2.prepare (maxDelay + 2);
        const size_t sz = (size_t) juce::nextPowerOfTwo (maxDelay + 2);
        delay.assign ((size_t) numCh, std::vector<float> (sz, 0.f));
        delayMask = sz - 1;
        rmsCoef = 1.f - std::exp (-1.f / (0.003f * (float) sr));   // 3 ms RMS window
        colSamples = std::max (1, (int) std::lround (sr * 0.008)); // 8 ms per history column
        postGain.reset (sr, 0.02);
        mixSm.reset (sr, 0.02);
        bypSm.reset (sr, 0.02);
        postGain.setCurrentAndTargetValue (1.f);
        mixSm.setCurrentAndTargetValue (1.f);
        bypSm.setCurrentAndTargetValue (0.f);
        currentDelay = -1;
        clearStates();
        for (int i = 0; i < kHistoryColumns; ++i) { histPeak[i].store (0.f); histGr[i].store (0.f); }
        histHead.store (0);
    }

    void reset() { clearStates(); }

    int computeDelaySamples (const Params& p) const
    {
        return p.lookaheadOn ? juce::jlimit (1, maxDelay, (int) std::lround (p.lookaheadMs * 0.001 * sr)) : 0;
    }
    int getLatencySamples() const noexcept { return currentDelay < 0 ? 0 : currentDelay; }

    void process (juce::AudioBuffer<float>& buffer, const Params& p)
    {
        const int nCh = juce::jmin (buffer.getNumChannels(), numCh);
        const int n   = buffer.getNumSamples();
        if (nCh == 0 || n == 0) return;

        const int D = computeDelaySamples (p);
        if (D != currentDelay) { currentDelay = D; clearStates(); }

        if (D > 0)
        {
            const int A  = juce::jlimit (2, D, (int) std::lround (p.smoothingMs * 0.001 * sr));
            const int L1 = std::max (1, A / 2);
            const int L2 = std::max (1, A - L1);
            ma1.setLength (L1);
            ma2.setLength (L2);
        }
        const float attCoef = std::exp (-1.f / (0.0005f * (float) sr));
        const float relCoef = std::exp (-1.f / (std::max (0.001f, p.smoothingMs * 0.001f) * (float) sr));

        postGain.setTargetValue (std::pow (10.f, p.postGainDb * 0.05f));
        mixSm.setTargetValue (juce::jlimit (0.f, 1.f, p.mix));
        bypSm.setTargetValue (p.bypass ? 1.f : 0.f);

        std::array<float*, kMaxChannels> data {};
        for (int c = 0; c < nCh; ++c) data[(size_t) c] = buffer.getWritePointer (c);

        float blockMinDb = 0.f;

        for (int i = 0; i < n; ++i)
        {
            // ---- detector (stereo-linked, on the *undelayed* signal) -------------
            float pk = 0.f, e = 0.f;
            for (int c = 0; c < nCh; ++c)
            {
                const float x = data[(size_t) c][i];
                pk = std::max (pk, std::abs (x));
                e += x * x;
            }
            rmsPow += rmsCoef * (e / (float) nCh - rmsPow);
            const float level = p.peakMode ? pk : std::sqrt (rmsPow) * 1.41421356f; // RMS scaled to peak-equivalent
            const float Ldb   = toDb (level);

            // ---- gain computer (dB, <= 0) -----------------------------------------
            const float grTarget = computeGrDb (Ldb, p.thresholdDb, p.targetDb, p.maxGrDb);

            // ---- smoothing ----------------------------------------------------------
            float gDb;
            if (D > 0)
            {
                const float m = slidingMin.push (grTarget, D + 1); // look ahead D samples
                gDb = ma2.process (ma1.process (m));               // triangular ramp, no release stage
            }
            else
            {
                const float coef = grTarget < zlGain ? attCoef : relCoef;
                zlGain = coef * zlGain + (1.f - coef) * grTarget;
                gDb = zlGain;
            }
            const float gLin = std::exp (gDb * 0.115129255f);
            blockMinDb = std::min (blockMinDb, gDb);

            // ---- apply to delayed audio -------------------------------------------
            const float pg = postGain.getNextValue();
            const float mx = mixSm.getNextValue();
            const float bp = bypSm.getNextValue();
            for (int c = 0; c < nCh; ++c)
            {
                auto& dl = delay[(size_t) c];
                dl[writePos & delayMask] = data[(size_t) c][i];
                const float dry = dl[(writePos - (size_t) D) & delayMask];
                const float wet = dry * gLin * pg;
                float out = dry + (wet - dry) * mx;
                out += (dry - out) * bp;
                data[(size_t) c][i] = out;
            }
            ++writePos;

            // ---- history for the display --------------------------------------------
            colPeak = std::max (colPeak, pk);
            colGr   = std::min (colGr, gDb);
            if (++colCount >= colSamples)
            {
                const int h = histHead.load (std::memory_order_relaxed);
                histPeak[h].store (colPeak, std::memory_order_relaxed);
                histGr[h].store (colGr, std::memory_order_relaxed);
                histHead.store ((h + 1) % kHistoryColumns, std::memory_order_release);
                colCount = 0; colPeak = 0.f; colGr = 0.f;
            }
        }
        grMeterDb.store (p.bypass ? 0.f : blockMinDb, std::memory_order_relaxed);
    }

    // Shared with the editor (lock-free).
    std::atomic<float> grMeterDb { 0.f };
    std::atomic<float> histPeak[kHistoryColumns];
    std::atomic<float> histGr[kHistoryColumns];
    std::atomic<int>   histHead { 0 };

    static float toDb (float lin) noexcept { return 20.f * std::log10 (lin > 1e-6f ? lin : 1e-6f); }

    // Soft-knee "pull toward target". Returns attenuation in dB (<= 0).
    static float computeGrDb (float Ldb, float thr, float tgt, float maxGr) noexcept
    {
        constexpr float kneeDb = 4.f;   // smooth engage over 4 dB above threshold
        constexpr float pull   = 0.8f;  // fraction of the excess (above target) removed
        const float over = Ldb - thr;
        if (over <= 0.f) return 0.f;
        const float t = std::min (over / kneeDb, 1.f);
        const float w = t * t * (3.f - 2.f * t);
        const float red = std::max (0.f, Ldb - tgt) * pull * w;
        return -std::min (red, maxGr);
    }

private:
    void clearStates()
    {
        for (auto& d : delay) std::fill (d.begin(), d.end(), 0.f);
        writePos = 0;
        slidingMin.reset();
        ma1.reset();
        ma2.reset();
        rmsPow = 0.f;
        zlGain = 0.f;
        colCount = 0; colPeak = 0.f; colGr = 0.f;
    }

    double sr = 44100.0;
    int numCh = 2, maxDelay = 0, currentDelay = -1, colSamples = 352, colCount = 0;
    std::vector<std::vector<float>> delay;
    size_t delayMask = 0, writePos = 0;
    SlidingMin slidingMin;
    MovingAverage ma1, ma2;
    float rmsCoef = 0.01f, rmsPow = 0.f, zlGain = 0.f, colPeak = 0.f, colGr = 0.f;
    juce::SmoothedValue<float> postGain, mixSm, bypSm;
};
} // namespace peaky
