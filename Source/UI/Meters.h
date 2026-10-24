#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Look.h"

namespace ui
{
// Meter di livello stereo (picco, -60..0 dBFS) con peak hold. push() va chiamato dal timer della GUI (~30 Hz).
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

        g.setFont (10.0f);
        for (float db : { 0.0f, -6.0f, -12.0f, -24.0f, -36.0f, -48.0f })
        {
            g.setColour (col::dim);
            g.drawText (juce::String ((int) db), scale.withY (dbToY (db, r) - 6.0f).withHeight (12.0f),
                        juce::Justification::centredRight, false);
        }

        for (int ch = 0; ch < 2; ++ch)
        {
            const juce::Rectangle<float> bar (r.getX() + (float) ch * (barW + gap), r.getY(), barW, r.getHeight());
            g.setColour (juce::Colour (0xff0f1115));
            g.fillRoundedRectangle (bar, 2.0f);

            juce::ColourGradient grad (juce::Colour (0xff3fbf6a), 0.0f, bar.getBottom(),
                                       juce::Colour (0xffe5484d), 0.0f, bar.getY(), false);
            grad.addColour (0.78, juce::Colour (0xffd9c43a));
            g.setGradientFill (grad);
            const float y = dbToY (levelDb[ch], bar);
            if (y < bar.getBottom())
                g.fillRect (juce::Rectangle<float> (bar.getX(), y, bar.getWidth(), bar.getBottom() - y));

            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillRect (bar.getX(), dbToY (holdDb[ch], bar), bar.getWidth(), 1.5f);
        }
    }

private:
    static float dbToY (float db, juce::Rectangle<float> r)
    {
        return juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, r.getBottom(), r.getY());
    }

    void update (int ch, float lin)
    {
        const float db = juce::Decibels::gainToDecibels (lin, -100.0f);
        levelDb[ch] = db > levelDb[ch] ? db : juce::jmax (db, levelDb[ch] - 1.5f);   // caduta ~45 dB/s

        if (db >= holdDb[ch])      { holdDb[ch] = db; holdFrames[ch] = 45; }          // hold ~1.5 s
        else if (holdFrames[ch] > 0) --holdFrames[ch];
        else                       holdDb[ch] = juce::jmax (levelDb[ch], holdDb[ch] - 0.7f);
    }

    float levelDb[2] { -100.0f, -100.0f }, holdDb[2] { -100.0f, -100.0f };
    int holdFrames[2] { 0, 0 };
};

// Meter di riduzione di guadagno: la barra scende dall'alto, 0..maxDb
class GRMeter : public juce::Component
{
public:
    GRMeter (float maxDbIn, juce::Colour accentIn) : maxDb (maxDbIn), accent (accentIn) {}

    void push (float grDb)
    {
        gr = grDb > gr ? grDb : juce::jmax (grDb, gr - 0.8f);   // caduta ~24 dB/s
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto readout = r.removeFromBottom (18.0f);
        auto scale = r.removeFromLeft (26.0f);
        const juce::Rectangle<float> bar = r.withWidth (juce::jmin (r.getWidth(), 22.0f));

        g.setFont (10.0f);
        for (int i = 0; i <= 4; ++i)
        {
            const float db = maxDb * (float) i / 4.0f;
            g.setColour (col::dim);
            g.drawText (i == 0 ? juce::String ("0") : "-" + juce::String ((int) std::round (db)),
                        scale.withY (dbToY (db, bar) - 6.0f).withHeight (12.0f),
                        juce::Justification::centredRight, false);
        }

        g.setColour (juce::Colour (0xff0f1115));
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (accent);
        g.fillRect (juce::Rectangle<float> (bar.getX(), bar.getY(), bar.getWidth(), dbToY (gr, bar) - bar.getY()));

        g.setColour (col::text);
        g.setFont (11.0f);
        g.drawText ("-" + juce::String (gr, 1) + " dB", readout.withLeft (0.0f), juce::Justification::centred, false);
    }

private:
    float dbToY (float db, juce::Rectangle<float> r) const
    {
        return juce::jmap (juce::jlimit (0.0f, maxDb, db), 0.0f, maxDb, r.getY(), r.getBottom());
    }

    float maxDb, gr = 0.0f;
    juce::Colour accent;
};
} // namespace ui
