#pragma once
#include <atomic>
#include "DspCommon.h"

namespace mc
{
// Compressore bus stile SSL G: feed-forward, detector in dB, knee morbido, sidechain HPF, auto-release a due stadi.
class BusCompressor
{
public:
    struct Params
    {
        float thresholdDb = -12.0f, ratio = 2.0f, attackMs = 10.0f, releaseMs = 300.0f;
        bool  autoRelease = true;
        float makeupDb = 0.0f, mix = 1.0f;       // mix 0..1 (compressione parallela)
    };

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        scHpf.makeHighPass (fs, 90.0, 0.70710678);
        makeup.prepare (fs, 20.0);
        reset();
    }

    void reset() { scHpf.reset(); grA = grFast = grSlow = 0.0; meter.store (0.0f); }

    void setParams (const Params& p)
    {
        thr = p.thresholdDb;  ratio = std::max (1.0f, p.ratio);
        atkC = coef (p.attackMs * 0.001);
        relC = coef (p.releaseMs * 0.001);
        relFastC = coef (0.1);
        relSlowC = coef (1.2);
        autoRel = p.autoRelease;
        makeup.setTarget (dbToGain (p.makeupDb));
        mix = std::clamp ((double) p.mix, 0.0, 1.0);
    }

    void process (float* const* ch, int numCh, int n)
    {
        numCh = std::min (numCh, 2);
        double blockMinGr = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const float l = ch[0][i];
            const float r = numCh > 1 ? ch[1][i] : l;

            // sidechain: HPF, poi link stereo sul picco maggiore
            const double dl = scHpf.process (0, l);
            const double dr = numCh > 1 ? scHpf.process (1, r) : dl;
            const double lvlDb = gainToDb (std::max (std::fabs (dl), std::fabs (dr)));

            // gain computer con knee morbido
            const double over = lvlDb - thr;
            double outDb;
            if (2.0 * over < -kneeW)             outDb = lvlDb;
            else if (2.0 * std::fabs (over) <= kneeW)
            {
                const double t = over + kneeW * 0.5;
                outDb = lvlDb + (1.0 / ratio - 1.0) * t * t / (2.0 * kneeW);
            }
            else                                  outDb = thr + over / ratio;
            const double target = outDb - lvlDb;     // <= 0 (riduzione di guadagno in dB)

            // ballistica nel dominio dB
            double gr;
            if (autoRel)
            {
                follow (grFast, target, atkC, relFastC);
                follow (grSlow, target, atkC, relSlowC);
                gr = 0.5 * (grFast + grSlow);
            }
            else
            {
                follow (grA, target, atkC, relC);
                gr = grA;
            }
            blockMinGr = std::min (blockMinGr, gr);

            const double g = dbToGain (gr) * makeup.next();
            for (int c = 0; c < numCh; ++c)
            {
                const double dry = ch[c][i];
                ch[c][i] = (float) (dry + (dry * g - dry) * mix);
            }
        }
        atomicMax (meter, (float) -blockMinGr);
    }

    // Massima riduzione (dB, positiva) dall'ultima lettura: chiamata dalla GUI
    float takeGainReductionDb() { return meter.exchange (0.0f); }

private:
    double coef (double sec) const { return std::exp (-1.0 / (std::max (sec, 1.0e-5) * fs)); }
    static void follow (double& s, double target, double aC, double rC)
    {
        const double c = target < s ? aC : rC;
        s = c * s + (1.0 - c) * target;
    }

    static constexpr double kneeW = 6.0;
    double fs = 44100.0, thr = -12.0, ratio = 2.0, mix = 1.0;
    double atkC = 0, relC = 0, relFastC = 0, relSlowC = 0;
    double grA = 0, grFast = 0, grSlow = 0;
    bool autoRel = true;
    Biquad scHpf;
    Smoother makeup;
    std::atomic<float> meter { 0.0f };
};
} // namespace mc
