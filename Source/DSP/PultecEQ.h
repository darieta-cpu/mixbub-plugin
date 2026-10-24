#pragma once
#include "DspCommon.h"

namespace mc
{
// EQ in stile Pultec EQP-1A. Punto di partenza: le curve vanno rifinite ad orecchio.
class PultecEQ
{
public:
    static constexpr double lfFreqs[4]       { 20, 30, 60, 100 };
    static constexpr double hfBoostFreqs[7]  { 3000, 4000, 5000, 8000, 10000, 12000, 16000 };
    static constexpr double hfAttenFreqs[3]  { 5000, 10000, 20000 };

    struct Params
    {
        int   lfFreqIdx = 2;
        float lfBoost = 0, lfAtten = 0;           // 0..10
        int   hfFreqIdx = 4;
        float hfBoost = 0, hfBw = 0.5f;           // 0..10, 0..1
        int   hfAttenIdx = 1;
        float hfAtten = 0;                        // 0..10
        float sat = 0;                            // 0..1
    };

    void prepare (double sampleRate) { fs = sampleRate; reset(); }
    void reset() { lfB.reset(); lfA.reset(); hfB.reset(); hfA.reset(); }

    void setParams (const Params& p)
    {
        const double lf = lfFreqs[clampIdx (p.lfFreqIdx, 4)];
        // Boost: shelf basso a f. Atten: shelf più largo con angolo più basso (f*0.4).
        // Usati insieme (trucco Pultec) lasciano una gobba attorno a f/2 e il fondo (sotto ~25 Hz) più stretto.
        lfB.makeLowShelf (fs, lf,        p.lfBoost * 1.3,  1.0);
        lfA.makeLowShelf (fs, lf * 0.4, -p.lfAtten * 1.5, 0.5);

        // Boost alte: campana con Q variabile dal controllo bandwidth (0 = stretto, 1 = largo)
        const double qv = 2.0 - 1.6 * (double) p.hfBw;
        hfB.makePeak      (fs, hfBoostFreqs[clampIdx (p.hfFreqIdx, 7)], qv, p.hfBoost * 1.6);
        hfA.makeHighShelf (fs, hfAttenFreqs[clampIdx (p.hfAttenIdx, 3)], -p.hfAtten * 2.0, 0.6);

        satK = 0.25 * (double) p.sat;
        satMax = satK > 1.0e-6 ? 1.0 / std::sqrt (3.0 * satK) : 0.0;
    }

    void process (float* const* ch, int numCh, int n)
    {
        numCh = std::min (numCh, 2);
        for (int c = 0; c < numCh; ++c)
        {
            float* d = ch[c];
            for (int i = 0; i < n; ++i)
            {
                float x = d[i];
                x = lfB.process (c, x);
                x = lfA.process (c, x);
                x = hfB.process (c, x);
                x = hfA.process (c, x);
                if (satK > 0.0)
                {
                    // soft clip cubico monotono: guadagno unitario a basso livello
                    const double xc = std::clamp ((double) x, -satMax, satMax);
                    x = (float) (xc - satK * xc * xc * xc);
                }
                d[i] = x;
            }
        }
    }

private:
    static int clampIdx (int i, int size) { return std::clamp (i, 0, size - 1); }

    double fs = 44100.0, satK = 0.0, satMax = 0.0;
    Biquad lfB, lfA, hfB, hfA;
};
} // namespace mc
