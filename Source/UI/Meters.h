#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Look.h"

namespace ui
{
// Meter di livello stereo a LED (picco, -60..0 dBFS) con peak hold.
// push() va chiamato dal timer della GUI (~60 Hz). Ogni LED si riempie in modo proporzionale: lettura precisa.
class LevelMeter : public juce::Component
{
public:
    void push (float linL, float linR)
    {
        update (0, linL);
        update (1, linR);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto scale = r.removeFromLeft (24.0f);
        const float gap = 4.0f;
        const float barW = (r.getWidth() - gap) * 0.5f;

        auto barRect = [&] (int ch)
        {
            return juce::Rectangle<float> (r.getX() + (float) ch * (barW + gap), r.getY(), barW, r.getHeight());
        };

        // scala a sinistra
        g.setFont (10.0f);
        for (float db : { 0.0f, -6.0f, -12.0f, -24.0f, -36.0f, -48.0f })
        {
            g.setColour (col::dim);
            g.drawText (juce::String ((int) db), scale.withY (yOf (db, barRect (0)) - 6.0f).withHeight (12.0f),
                        juce::Justification::centredRight, false);
        }

        for (int ch = 0; ch < 2; ++ch)
        {
            const auto bar = barRect (ch);
            g.setColour (juce::Colour (0xff0a0a0b));
            g.fillRoundedRectangle (bar, 2.0f);
            g.setColour (juce::Colour (0xff55575c));
            g.drawRoundedRectangle (bar.reduced (0.5f), 2.0f, 1.0f);

            const auto inner = bar.reduced (2.0f);

            for (int i = 0; i < numSeg; ++i)
            {
                const float lo = minDb + step * (float) i;
                const juce::Colour c = lo >= -3.0f  ? juce::Colour (0xffff3b30)
                                     : lo >= -9.0f  ? juce::Colour (0xffff9800)
                                     : lo >= -18.0f ? juce::Colour (0xffe6d23a)
                                                    : juce::Colour (0xff3ddc4a);
                const auto s = segRect (i, inner);

                g.setColour (c.withAlpha (0.13f));
                g.fillRect (s);

                const float f = juce::jlimit (0.0f, 1.0f, (levelDb[ch] - lo) / step);
                if (f > 0.0f)
                {
                    g.setColour (c);
                    g.fillRect (s.getX(), s.getBottom() - s.getHeight() * f, s.getWidth(), s.getHeight() * f);
                }
            }

            if (holdDb[ch] > minDb)
            {
                g.setColour (holdDb[ch] >= -0.5f ? juce::Colour (0xffff3b30) : juce::Colours::white);
                g.fillRect (inner.getX(), yOf (holdDb[ch], inner) - 1.0f, inner.getWidth(), 2.0f);
            }
        }
    }

private:
    static constexpr int numSeg = 20;          // 3 dB ciascuno
    static constexpr float minDb = -60.0f, step = 3.0f;

    static juce::Rectangle<float> segRect (int i, juce::Rectangle<float> bar)
    {
        const float segH = bar.getHeight() / (float) numSeg;
        return { bar.getX(), bar.getBottom() - (float) (i + 1) * segH + 1.0f, bar.getWidth(), segH - 2.0f };
    }

    // posizione verticale esatta di un valore in dB (continua dentro ogni LED)
    static float yOf (float db, juce::Rectangle<float> bar)
    {
        const float pos = juce::jlimit (0.0f, (float) numSeg - 0.0001f, (db - minDb) / step);
        const int i = (int) std::floor (pos);
        const auto s = segRect (i, bar);
        return s.getBottom() - s.getHeight() * (pos - (float) i);
    }

    void update (int ch, float lin)
    {
        const float db = juce::Decibels::gainToDecibels (lin, -100.0f);
        levelDb[ch] = db > levelDb[ch] ? db : juce::jmax (db, levelDb[ch] - 0.75f);   // caduta ~45 dB/s a 60 Hz

        if (db >= holdDb[ch])        { holdDb[ch] = db; holdFrames[ch] = 90; }          // hold ~1.5 s
        else if (holdFrames[ch] > 0) --holdFrames[ch];
        else                         holdDb[ch] = juce::jmax (levelDb[ch], holdDb[ch] - 0.35f);
    }

    float levelDb[2] { -100.0f, -100.0f }, holdDb[2] { -100.0f, -100.0f };
    int holdFrames[2] { 0, 0 };
};

// Meter di riduzione di guadagno a LED: si riempie dall'alto, 0..maxDb
class GRMeter : public juce::Component
{
public:
    GRMeter (float maxDbIn, juce::Colour) : maxDb (maxDbIn),
                                           numSeg (juce::jmax (4, juce::roundToInt (maxDbIn / 2.0f))),
                                           step (maxDbIn / (float) juce::jmax (4, juce::roundToInt (maxDbIn / 2.0f))) {}

    void push (float grDb)
    {
        gr = grDb > gr ? grDb : juce::jmax (grDb, gr - 0.4f);   // caduta ~24 dB/s a 60 Hz
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto readout = r.removeFromBottom (18.0f);
        auto scale = r.removeFromLeft (26.0f);
        const juce::Rectangle<float> bar = r.withWidth (juce::jmin (r.getWidth(), 22.0f));
        const auto inner = bar.reduced (2.0f);

        g.setFont (10.0f);
        for (int i = 0; i <= 4; ++i)
        {
            const float db = maxDb * (float) i / 4.0f;
            g.setColour (col::dim);
            g.drawText (i == 0 ? juce::String ("0") : "-" + juce::String ((int) std::round (db)),
                        scale.withY (yOf (db, inner) - 6.0f).withHeight (12.0f),
                        juce::Justification::centredRight, false);
        }

        g.setColour (juce::Colour (0xff0a0a0b));
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (juce::Colour (0xff55575c));
        g.drawRoundedRectangle (bar.reduced (0.5f), 2.0f, 1.0f);

        for (int i = 0; i < numSeg; ++i)
        {
            const juce::Colour c = (float) i >= (float) numSeg * 0.7f ? juce::Colour (0xffff3b30)
                                                                       : juce::Colour (0xffff9800);
            const auto s = segRect (i, inner);

            g.setColour (c.withAlpha (0.13f));
            g.fillRect (s);

            const float f = juce::jlimit (0.0f, 1.0f, (gr - (float) i * step) / step);
            if (f > 0.0f)
            {
                g.setColour (c);
                g.fillRect (s.getX(), s.getY(), s.getWidth(), s.getHeight() * f);
            }
        }

        g.setColour (col::text);
        g.setFont (11.0f);
        g.drawText ("-" + juce::String (gr, 1) + " dB", readout.withLeft (0.0f), juce::Justification::centred, false);
    }

private:
    juce::Rectangle<float> segRect (int i, juce::Rectangle<float> bar) const
    {
        const float segH = bar.getHeight() / (float) numSeg;
        return { bar.getX(), bar.getY() + (float) i * segH + 1.0f, bar.getWidth(), segH - 2.0f };
    }

    float yOf (float db, juce::Rectangle<float> bar) const
    {
        const float pos = juce::jlimit (0.0f, (float) numSeg - 0.0001f, db / step);
        const int i = (int) std::floor (pos);
        const auto s = segRect (i, bar);
        return s.getY() + s.getHeight() * (pos - (float) i);
    }

    float maxDb;
    int numSeg;
    float step;
    float gr = 0.0f;
};
} // namespace ui
