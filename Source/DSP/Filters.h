#pragma once
#include "DspCommon.h"

namespace mc
{
// HPF + LPF Butterworth, 12 o 24 dB/ott, con bypass automatico agli estremi (20 Hz / 20 kHz)
class FilterModule
{
public:
    void prepare (double sampleRate) { fs = sampleRate; reset(); }

    void reset()
    {
        for (auto& b : hp) b.reset();
        for (auto& b : lp) b.reset();
    }

    void setParams (float hpfHz, int hpfSlope, float lpfHz, int lpfSlope)
    {
        const bool newHpOn = hpfHz > 20.5f;
        const bool newLpOn = lpfHz < 19900.0f;
        if (newHpOn && ! hpOn) for (auto& b : hp) b.reset();
        if (newLpOn && ! lpOn) for (auto& b : lp) b.reset();
        hpOn = newHpOn;  lpOn = newLpOn;

        hpStages = hpfSlope == 0 ? 1 : 2;
        lpStages = lpfSlope == 0 ? 1 : 2;

        if (hpOn) for (int s = 0; s < hpStages; ++s) hp[s].makeHighPass (fs, hpfHz, q (hpStages, s));
        if (lpOn) for (int s = 0; s < lpStages; ++s) lp[s].makeLowPass  (fs, lpfHz, q (lpStages, s));
    }

    void process (float* const* ch, int numCh, int n)
    {
        if (! hpOn && ! lpOn) return;
        numCh = std::min (numCh, 2);
        for (int c = 0; c < numCh; ++c)
        {
            float* d = ch[c];
            for (int i = 0; i < n; ++i)
            {
                float x = d[i];
                if (hpOn) for (int s = 0; s < hpStages; ++s) x = hp[s].process (c, x);
                if (lpOn) for (int s = 0; s < lpStages; ++s) x = lp[s].process (c, x);
                d[i] = x;
            }
        }
    }

private:
    // Q dei biquad in cascata per Butterworth del 2° e 4° ordine
    static double q (int stages, int s)
    {
        static constexpr double q24[] { 0.54119610, 1.30656296 };
        return stages == 1 ? 0.70710678 : q24[s];
    }

    double fs = 44100.0;
    Biquad hp[2], lp[2];
    bool hpOn = false, lpOn = false;
    int hpStages = 1, lpStages = 1;
};
} // namespace mc
