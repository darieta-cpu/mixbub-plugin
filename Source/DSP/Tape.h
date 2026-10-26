#pragma once
#include <vector>
#include "DspCommon.h"

namespace mc
{
// Oversampler 2x con filtro halfband FIR a 67 tap (Kaiser), implementazione polifase.
// Latenza complessiva (su + giu): 33 campioni a frequenza originale, indipendente dal sample rate.
class HalfbandOversampler2x
{
public:
    static constexpr int kLatency = 33;

    HalfbandOversampler2x() { design(); reset(); }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            std::fill (std::begin (upBuf[c]), std::end (upBuf[c]), 0.0f);
            std::fill (std::begin (dnE[c]),   std::end (dnE[c]),   0.0f);
            std::fill (std::begin (dnO[c]),   std::end (dnO[c]),   0.0f);
            upW[c] = dnW[c] = 0;
        }
    }

    // un campione in ingresso -> due campioni a 2x
    inline void up (int ch, float x, float& y0, float& y1) noexcept
    {
        int& w = upW[ch];
        upBuf[ch][w] = x;
        float acc = 0.0f;
        for (int k = 0; k < kTaps; ++k) acc += coef[k] * upBuf[ch][(w - k) & 63];
        y0 = 2.0f * acc;                   // fase pari: filtro polifase
        y1 = upBuf[ch][(w - 16) & 63];     // fase dispari: solo il tap centrale (ritardo puro)
        w = (w + 1) & 63;
    }

    // due campioni a 2x -> un campione a frequenza originale
    inline float down (int ch, float z0, float z1) noexcept
    {
        int& w = dnW[ch];
        dnE[ch][w] = z0;
        dnO[ch][w] = z1;
        float acc = 0.5f * dnO[ch][(w - 17) & 63];
        for (int k = 0; k < kTaps; ++k) acc += coef[k] * dnE[ch][(w - k) & 63];
        w = (w + 1) & 63;
        return acc;
    }

private:
    static constexpr int kTaps = 34;

    static double bessel0 (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 40; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; }
        return sum;
    }

    void design()
    {
        constexpr int C = 33;                       // tap centrale (0.5)
        constexpr double beta = 7.0;
        double h[2 * kTaps] {};
        double oddSum = 0.0;
        for (int k = 0; k < kTaps; ++k)             // tap pari j = 2k, offset m = j - C (dispari)
        {
            const double m = (double) (2 * k - C);
            const double r = m / (double) (C + 1);
            const double win = bessel0 (beta * std::sqrt (1.0 - r * r)) / bessel0 (beta);
            h[k] = std::sin (kPi * m * 0.5) / (kPi * m) * win;
            oddSum += h[k];
        }
        const double scale = 0.5 / oddSum;          // somma totale dei tap = 1 (guadagno unitario in DC)
        for (int k = 0; k < kTaps; ++k) coef[k] = (float) (h[k] * scale);
    }

    float coef[kTaps] {};
    float upBuf[2][64] {}, dnE[2][64] {}, dnO[2][64] {};
    int upW[2] { 0, 0 }, dnW[2] { 0, 0 };
};

// Emulazione di nastro analogico (approssimazione pratica):
// saturazione asimmetrica level-dependent a 2x -> DC blocker -> head bump -> perdita di alte (tone) -> rolloff di velocità.
// Il dry è sempre ritardato di 33 campioni (anche con il modulo spento) così la latenza resta costante.
class TapeModule
{
public:
    struct Params
    {
        bool  on = true;
        float drive = 3.0f;        // 0..10
        float bump = 5.0f;         // 0..10
        float tone = 2.0f;         // 0..10 (perdita di alte)
        int   speedIdx = 1;        // 0 = 7.5 ips, 1 = 15 ips, 2 = 30 ips
        float mix = 1.0f;          // 0..1
        float outDb = 0.0f;
    };

    static constexpr int kLatency = HalfbandOversampler2x::kLatency;
    int getLatencySamples() const { return kLatency; }

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        os.reset();
        for (auto& d : dly) d.assign ((size_t) kLatency, 0.0f);
        dpos = 0;
        bump.reset(); tone.reset(); lpf.reset();
        for (int c = 0; c < 2; ++c) dcX[c] = dcY[c] = 0.0;
        gS.prepare (fs, 20.0);  mixS.prepare (fs, 20.0);  outS.prepare (fs, 20.0);
        first = true;
    }

    void setParams (const Params& p)
    {
        const int s = std::clamp (p.speedIdx, 0, 2);
        const double fs2 = 2.0 * fs;
        bump.makePeak      (fs2, bumpFreq[s], 1.0, (double) p.bump * bumpDbPerUnit[s]);
        tone.makeHighShelf (fs2, toneFreq[s], -(double) p.tone * 1.2, 0.7);
        lpf.makeLowPass    (fs2, speedLpf[s], 0.6);
        dcR = 1.0 - 2.0 * kPi * 8.0 / fs2;

        gS.setTarget   (dbToGain ((double) p.drive * 1.5 - 6.0));   // drive 0 = -6 dB (quasi pulito), 10 = +9 dB
        mixS.setTarget (p.on ? std::clamp ((double) p.mix, 0.0, 1.0) : 0.0);
        outS.setTarget (p.on ? dbToGain ((double) p.outDb) : 1.0);
        if (first) { gS.snap (gS.target); mixS.snap (mixS.target); outS.snap (outS.target); first = false; }
    }

    void process (float* const* ch, int numCh, int n)
    {
        numCh = std::min (numCh, 2);
        for (int i = 0; i < n; ++i)
        {
            const double g = gS.next();
            const double comp = kRef / sat (g * kRef);   // compensa il livello a circa -16 dBFS
            const double m = mixS.next();
            const double outG = outS.next();
            const bool wetActive = mixS.target > 0.0 || m > 1.0e-4;

            for (int c = 0; c < numCh; ++c)
            {
                const float x = ch[c][i];
                const float dry = dly[c][(size_t) dpos];
                dly[c][(size_t) dpos] = x;

                float wet = 0.0f;
                if (wetActive)
                {
                    float u0, u1;
                    os.up (c, x, u0, u1);
                    const float z0 = shape (c, u0, g, comp);
                    const float z1 = shape (c, u1, g, comp);
                    wet = os.down (c, z0, z1);
                }
                ch[c][i] = (float) ((dry * (1.0 - m) + wet * m) * outG);
            }
            if (++dpos >= kLatency) dpos = 0;
        }
    }

private:
    inline double sat (double v) const { return (std::tanh (v + kAsym) - tanhAsym) * invSlope; }

    inline float shape (int c, float u, double g, double comp) noexcept
    {
        const double y = sat (g * (double) u) * comp;
        const double o = y - dcX[c] + dcR * dcY[c];      // DC blocker (l'asimmetria genera DC)
        dcX[c] = y;  dcY[c] = o;
        float f = (float) o;
        f = bump.process (c, f);
        f = tone.process (c, f);
        f = lpf.process (c, f);
        return f;
    }

    static constexpr double kAsym = 0.15, kRef = 0.15;
    static constexpr double bumpFreq[3]       { 110.0, 60.0, 35.0 };
    static constexpr double bumpDbPerUnit[3]  { 0.7, 0.5, 0.35 };
    static constexpr double toneFreq[3]       { 6000.0, 9000.0, 12000.0 };
    static constexpr double speedLpf[3]       { 14000.0, 18000.0, 21000.0 };

    double fs = 44100.0, dcR = 0.999;
    const double tanhAsym = std::tanh (kAsym);
    const double invSlope = 1.0 / (1.0 - std::tanh (kAsym) * std::tanh (kAsym));
    double dcX[2] { 0, 0 }, dcY[2] { 0, 0 };
    HalfbandOversampler2x os;
    Biquad bump, tone, lpf;
    std::vector<float> dly[2];
    int dpos = 0;
    bool first = true;
    Smoother gS, mixS, outS;
};
} // namespace mc
