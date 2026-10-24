#pragma once
#include <algorithm>
#include <cmath>

// DSP "puro" C++17: nessuna dipendenza da JUCE, così si può testare da riga di comando.
namespace mc
{
constexpr double kPi = 3.14159265358979323846;

inline double dbToGain (double db) { return std::pow (10.0, db * 0.05); }
inline double gainToDb (double g)  { return 20.0 * std::log10 (std::max (g, 1.0e-9)); }

// Smoother a un polo per evitare scricchiolii sui parametri
struct Smoother
{
    double current = 1.0, target = 1.0, coef = 0.0;
    void prepare (double fs, double ms) { coef = std::exp (-1.0 / (0.001 * ms * fs)); }
    void snap (double v)      { current = target = v; }
    void setTarget (double v) { target = v; }
    double next()             { current = target + coef * (current - target); return current; }
};

// Biquad (RBJ cookbook), forma TDF-II, stato in double, fino a 2 canali
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] { 0, 0 }, z2[2] { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }

    inline float process (int ch, float x) noexcept
    {
        const double in  = x;
        const double out = b0 * in + z1[ch];
        z1[ch] = b1 * in - a1 * out + z2[ch];
        z2[ch] = b2 * in - a2 * out;
        return (float) out;
    }

    void makeLowPass (double fs, double f, double q)
    {
        f = clampFreq (fs, f);
        const double w = 2.0 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al);
    }

    void makeHighPass (double fs, double f, double q)
    {
        f = clampFreq (fs, f);
        const double w = 2.0 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
    }

    void makePeak (double fs, double f, double q, double gainDb)
    {
        f = clampFreq (fs, f);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        set (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
    }

    // S <= 1 (S = 1: pendenza massima senza overshoot)
    void makeLowShelf (double fs, double f, double gainDb, double S)
    {
        f = clampFreq (fs, f);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * f / fs, cw = std::cos (w), sw = std::sin (w);
        const double al = sw / 2.0 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2);
        const double t = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * cw + t),
             2 * A * ((A - 1) - (A + 1) * cw),
             A * ((A + 1) - (A - 1) * cw - t),
             (A + 1) + (A - 1) * cw + t,
             -2 * ((A - 1) + (A + 1) * cw),
             (A + 1) + (A - 1) * cw - t);
    }

    void makeHighShelf (double fs, double f, double gainDb, double S)
    {
        f = clampFreq (fs, f);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * f / fs, cw = std::cos (w), sw = std::sin (w);
        const double al = sw / 2.0 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2);
        const double t = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * cw + t),
             -2 * A * ((A - 1) + (A + 1) * cw),
             A * ((A + 1) + (A - 1) * cw - t),
             (A + 1) - (A - 1) * cw + t,
             2 * ((A - 1) - (A + 1) * cw),
             (A + 1) - (A - 1) * cw - t);
    }

private:
    static double clampFreq (double fs, double f) { return std::clamp (f, 5.0, fs * 0.45); }

    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};
} // namespace mc
