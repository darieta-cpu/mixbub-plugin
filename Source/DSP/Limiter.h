#pragma once
#include <vector>
#include "DspCommon.h"

namespace mc
{
// Limiter lookahead stile LoudMax: "Threshold" spinge il segnale dentro, "Ceiling" è il tetto in uscita.
// Algoritmo: guadagno richiesto -> minimo su finestra D -> release esponenziale -> media mobile D.
// Il segnale è ritardato di D-1 campioni, così il guadagno raggiunge il valore giusto prima del picco.
// (Non gestisce ancora i true peak: è il passo successivo con l'oversampling.)
class Limiter
{
public:
    struct Params
    {
        float thresholdDb = 0.0f, ceilingDb = -0.3f, releaseMs = 100.0f;
        bool  bypass = false;
    };

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        D = std::max (2, (int) std::lround (0.0015 * fs));   // lookahead ~1.5 ms
        delayLen = D - 1;
        for (auto& d : delay) d.assign ((size_t) delayLen, 0.0f);
        rbuf.assign ((size_t) D, 1.0); mbuf.assign ((size_t) D, 1.0);
        pos = wr = 0;  env = 1.0;
        gIn.prepare (fs, 20.0);  gIn.snap (1.0);
    }

    int getLatencySamples() const { return delayLen; }

    void setParams (const Params& p)
    {
        bypass = p.bypass;
        ceil = dbToGain (p.ceilingDb);
        gIn.setTarget (bypass ? 1.0 : dbToGain (-p.thresholdDb));
        relC = std::exp (-1.0 / (std::max (0.001, p.releaseMs * 0.001) * fs));
    }

    void process (float* const* ch, int numCh, int n)
    {
        numCh = std::min (numCh, 2);
        for (int i = 0; i < n; ++i)
        {
            const double g = gIn.next();
            double peak = 0.0;
            float delayed[2] { 0, 0 };

            for (int c = 0; c < numCh; ++c)
            {
                const double xs = ch[c][i] * g;
                peak = std::max (peak, std::fabs (xs));
                delayed[c] = delay[c][(size_t) pos];
                delay[c][(size_t) pos] = (float) xs;
            }
            if (++pos >= delayLen) pos = 0;

            if (bypass)
            {
                rbuf[(size_t) wr] = 1.0;  mbuf[(size_t) wr] = 1.0;  env = 1.0;
                if (++wr >= D) wr = 0;
                for (int c = 0; c < numCh; ++c) ch[c][i] = delayed[c];
                continue;
            }

            rbuf[(size_t) wr] = peak > ceil ? ceil / peak : 1.0;
            double m = 1.0;
            for (int k = 0; k < D; ++k) m = std::min (m, rbuf[(size_t) k]);

            env = (m < env) ? m : relC * env + (1.0 - relC) * m;
            mbuf[(size_t) wr] = env;

            double sum = 0.0;
            for (int k = 0; k < D; ++k) sum += mbuf[(size_t) k];
            const double gain = sum / (double) D;
            if (++wr >= D) wr = 0;

            for (int c = 0; c < numCh; ++c)
                ch[c][i] = (float) std::clamp ((double) delayed[c] * gain, -ceil, ceil);   // clamp di sicurezza
        }
    }

private:
    double fs = 44100.0, ceil = 0.966, relC = 0.9, env = 1.0;
    int D = 66, delayLen = 65, pos = 0, wr = 0;
    bool bypass = false;
    std::vector<float> delay[2];
    std::vector<double> rbuf, mbuf;
    Smoother gIn;
};
} // namespace mc
