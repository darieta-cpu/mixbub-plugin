#include "PluginEditor.h"

using namespace ui;

MainPanel::MainPanel (MasterChainProcessor& p)
    : proc (p),
      gain (p.apvts, "in_gain", "INPUT GAIN", col::input),
      tapeDrive (p.apvts, "tape_drive", "DRIVE", col::tape),
      tapeBump (p.apvts, "tape_bump", "BUMP", col::tape),
      tapeTone (p.apvts, "tape_tone", "TONE", col::tape),
      tapeMix (p.apvts, "tape_mix", "MIX", col::tape),
      tapeOut (p.apvts, "tape_out", "OUTPUT", col::tape),
      tapeSpeed (p.apvts, "tape_speed", "Speed"),
      tapePos (p.apvts, "tape_pos", "Position"),
      hpf (p.apvts, "hpf_freq", "HPF", col::filters),
      lpf (p.apvts, "lpf_freq", "LPF", col::filters),
      hpfSlope (p.apvts, "hpf_slope", "HPF Slope"),
      lpfSlope (p.apvts, "lpf_slope", "LPF Slope"),
      lfFreq (p.apvts, "eq_lf_freq", "Low Freq"),
      hfFreq (p.apvts, "eq_hf_freq", "Boost Freq"),
      hfAtFreq (p.apvts, "eq_hf_atfreq", "Atten Freq"),
      lfBoost (p.apvts, "eq_lf_boost", "LF BOOST", col::eq),
      lfAtten (p.apvts, "eq_lf_atten", "LF ATTEN", col::eq),
      hfBoost (p.apvts, "eq_hf_boost", "HF BOOST", col::eq),
      hfBw (p.apvts, "eq_hf_bw", "BANDWIDTH", col::eq),
      hfAtten (p.apvts, "eq_hf_atten", "HF ATTEN", col::eq),
      sat (p.apvts, "eq_sat", "TUBE SAT", col::eq),
      thr (p.apvts, "comp_thresh", "THRESHOLD", col::comp),
      makeup (p.apvts, "comp_makeup", "MAKEUP", col::comp),
      mix (p.apvts, "comp_mix", "MIX", col::comp),
      ratio (p.apvts, "comp_ratio", "Ratio"),
      attack (p.apvts, "comp_attack", "Attack"),
      release (p.apvts, "comp_release", "Release"),
      compGr (20.0f, col::comp),
      limThr (p.apvts, "lim_thresh", "THRESHOLD", col::limiter),
      limCeil (p.apvts, "lim_ceiling", "CEILING", col::limiter),
      limRel (p.apvts, "lim_release", "RELEASE", col::limiter),
      limGr (12.0f, col::limiter)
{
    setLookAndFeel (&look);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &gain, &inMeter,
             &tapeOn, &tapeDrive, &tapeBump, &tapeTone, &tapeMix, &tapeOut, &tapeSpeed, &tapePos,
             &filtOn, &hpf, &lpf, &hpfSlope, &lpfSlope,
             &eqOn, &lfFreq, &hfFreq, &hfAtFreq, &lfBoost, &lfAtten, &hfBoost, &hfBw, &hfAtten, &sat,
             &compOn, &thr, &makeup, &mix, &ratio, &attack, &release, &compGr,
             &limOn, &limThr, &limCeil, &limRel, &limGr })
        addAndMakeVisible (c);

    setupToggle (tapeOn, tapeAtt, "tape_on", col::tape);
    setupToggle (filtOn, filtAtt, "filt_on", col::filters);
    setupToggle (eqOn,   eqAtt,   "eq_on",   col::eq);
    setupToggle (compOn, compAtt, "comp_on", col::comp);
    setupToggle (limOn,  limAtt,  "lim_on",  col::limiter);

    setSize (layout::baseW, layout::baseH);
    startTimerHz (60);
}

MainPanel::~MainPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MainPanel::setupToggle (juce::ToggleButton& b, std::unique_ptr<APVTS::ButtonAttachment>& att,
                             const char* id, juce::Colour accent)
{
    b.setButtonText ("ON");
    b.setColour (juce::ToggleButton::tickColourId, accent);
    att = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, id, b);
}

void MainPanel::timerCallback()
{
    inMeter.push (proc.takeInputPeak (0), proc.takeInputPeak (1));
    compGr.push (proc.takeCompGainReductionDb());
    limGr.push (proc.takeLimiterGainReductionDb());
}

void MainPanel::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, juce::Colour)
{
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRoundedRectangle (r.toFloat(), 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 4.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawHorizontalLine (r.getY() + 36, (float) r.getX() + 10.0f, (float) r.getRight() - 10.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText (title, r.reduced (12, 10).removeFromTop (24), juce::Justification::centredLeft, false);
}

void MainPanel::paint (juce::Graphics& g)
{
    const int w = getWidth(), h = getHeight();

    // pannello frontale
    juce::ColourGradient bg (juce::Colour (0xff2a2b2e), 0.0f, 0.0f,
                             juce::Colour (0xff151618), 0.0f, (float) h, false);
    g.setGradientFill (bg);
    g.fillAll();

    // orecchie da rack con viti
    auto screw = [&g] (float cx, float cy)
    {
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillEllipse (cx - 6.0f, cy - 6.0f, 12.0f, 12.0f);
        g.setColour (juce::Colour (0xff8b8d93));
        g.fillEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        juce::Path slot;
        slot.addRectangle (-4.0f, -0.8f, 8.0f, 1.6f);
        slot.applyTransform (juce::AffineTransform::rotation (0.6f).translated (cx, cy));
        g.setColour (juce::Colour (0xff2d2e32));
        g.fillPath (slot);
    };

    for (int side = 0; side < 2; ++side)
    {
        const int ex = side == 0 ? 0 : w - 22;
        g.setColour (juce::Colour (0xff333438));
        g.fillRect (ex, 0, 22, h);
        g.setColour (juce::Colour (0xff17181a));
        g.drawVerticalLine (side == 0 ? 21 : w - 22, 0.0f, (float) h);
        screw ((float) ex + 11.0f, 22.0f);
        screw ((float) ex + 11.0f, (float) h - 22.0f);
    }

    // intestazione
    g.setColour (juce::Colour (0xfff2f2f2));
    g.setFont (juce::FontOptions (22.0f).withStyle ("Bold"));
    g.drawText ("MIXBUS CHAIN", 30, 8, 300, 30, juce::Justification::centredLeft, false);

    g.setColour (col::dim.withAlpha (0.8f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("INPUT  >  TAPE  >  FILTERS  >  PULTEC EQ  >  BUS COMPRESSOR  >  LIMITER", 300, 8, 800, 30,
                juce::Justification::centredLeft, false);

    g.setColour (col::accent);
    g.setFont (juce::FontOptions (22.0f).withStyle ("Bold"));
    g.drawText ("DaniArieta", w - 30 - 260, 8, 260, 30, juce::Justification::centredRight, false);

    g.setColour (col::accent);
    g.fillRect (30, 42, w - 60, 2);

    drawPanel (g, inputR, "INPUT",          col::input);
    drawPanel (g, tapeR,  "TAPE",           col::tape);
    drawPanel (g, filtR,  "FILTERS",        col::filters);
    drawPanel (g, eqR,    "PULTEC EQ",      col::eq);
    drawPanel (g, compR,  "BUS COMPRESSOR", col::comp);
    drawPanel (g, limR,   "LIMITER",        col::limiter);
}

void MainPanel::resized()
{
    auto area = getLocalBounds().reduced (26, 10);
    area.removeFromTop (44);

    inputR = area.removeFromLeft (110);  area.removeFromLeft (10);
    tapeR  = area.removeFromLeft (260);  area.removeFromLeft (10);
    filtR  = area.removeFromLeft (190);  area.removeFromLeft (10);
    eqR    = area.removeFromLeft (330);  area.removeFromLeft (10);
    compR  = area.removeFromLeft (260);  area.removeFromLeft (10);
    limR   = area;

    // contenuto di un pannello: sotto la barra del titolo; il bottone ON sta a destra del titolo
    auto content = [] (juce::Rectangle<int> panel, juce::ToggleButton* b)
    {
        auto r = panel.reduced (10);
        auto head = r.removeFromTop (24);
        if (b != nullptr) b->setBounds (head.removeFromRight (40).reduced (0, 3));
        return r.withTrimmedTop (6);
    };
    const int knobH = 112, comboH = 40;

    // Input
    {
        auto c = content (inputR, nullptr);
        gain.setBounds (c.removeFromTop (knobH));
        inMeter.setBounds (c.withSizeKeepingCentre (72, c.getHeight()).reduced (0, 6));
    }

    // Nastro: 3 colonne x 2 righe + interruttore di posizione
    {
        auto c = content (tapeR, &tapeOn);
        const int w = c.getWidth() / 3;
        auto r1 = c.removeFromTop (knobH + 6);
        tapeDrive.setBounds (r1.removeFromLeft (w).withSizeKeepingCentre (w, knobH));
        tapeBump.setBounds  (r1.removeFromLeft (w).withSizeKeepingCentre (w, knobH));
        tapeTone.setBounds  (r1.withSizeKeepingCentre (w, knobH));
        auto r2 = c.removeFromTop (knobH + 6);
        tapeMix.setBounds   (r2.removeFromLeft (w).withSizeKeepingCentre (w, knobH));
        tapeOut.setBounds   (r2.removeFromLeft (w).withSizeKeepingCentre (w, knobH));
        tapeSpeed.setBounds (r2.withSizeKeepingCentre (w - 4, comboH));
        tapePos.setBounds   (c.removeFromTop (comboH + 14).withSizeKeepingCentre (c.getWidth() - 10, comboH));
    }

    // Filtri: due blocchi (manopola + pendenza)
    {
        auto c = content (filtR, &filtOn);
        const int half = c.getHeight() / 2;
        auto block = [&] (juce::Rectangle<int> r, Knob& k, Combo& s)
        {
            k.setBounds (r.removeFromTop (knobH).withSizeKeepingCentre (130, knobH));
            s.setBounds (r.removeFromTop (comboH).withSizeKeepingCentre (130, comboH));
        };
        block (c.removeFromTop (half), hpf, hpfSlope);
        block (c, lpf, lpfSlope);
    }

    // Pultec: tre righe (frequenza + due manopole)
    {
        auto c = content (eqR, &eqOn);
        const int rowH = c.getHeight() / 3;
        auto row = [&] (juce::Rectangle<int> r, Combo& f, Knob& k1, Knob& k2)
        {
            const int w = r.getWidth() / 3;
            f.setBounds  (r.removeFromLeft (w).withSizeKeepingCentre (w - 12, comboH));
            k1.setBounds (r.removeFromLeft (w).withSizeKeepingCentre (w, knobH));
            k2.setBounds (r.removeFromLeft (w).withSizeKeepingCentre (w, knobH));
        };
        row (c.removeFromTop (rowH), lfFreq,   lfBoost, lfAtten);
        row (c.removeFromTop (rowH), hfFreq,   hfBoost, hfBw);
        row (c,                      hfAtFreq, hfAtten, sat);
    }

    // Compressore
    {
        auto c = content (compR, &compOn);
        compGr.setBounds (c.removeFromRight (64));
        c.removeFromRight (6);
        auto r1 = c.removeFromTop (knobH + 6);
        const int w = c.getWidth() / 2;
        thr.setBounds    (r1.removeFromLeft (w));
        makeup.setBounds (r1);
        auto r2 = c.removeFromTop (knobH + 6);
        mix.setBounds   (r2.removeFromLeft (w));
        ratio.setBounds (r2.withSizeKeepingCentre (w - 8, comboH));
        auto r3 = c.removeFromTop (comboH + 10);
        attack.setBounds  (r3.removeFromLeft (w).withSizeKeepingCentre (w - 8, comboH));
        release.setBounds (r3.withSizeKeepingCentre (w - 8, comboH));
    }

    // Limiter
    {
        auto c = content (limR, &limOn);
        limGr.setBounds (c.removeFromRight (64));
        c.removeFromRight (6);
        limThr.setBounds  (c.removeFromTop (knobH + 6));
        limCeil.setBounds (c.removeFromTop (knobH + 6));
        limRel.setBounds  (c.removeFromTop (knobH + 6));
    }
}

//==============================================================================
MasterChainEditor::MasterChainEditor (MasterChainProcessor& p)
    : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    panel.setBounds (0, 0, layout::baseW, layout::baseH);

    setResizable (true, true);
    setResizeLimits (layout::baseW / 2, layout::baseH / 2, layout::baseW * 2, layout::baseH * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) layout::baseW / (double) layout::baseH);
    setSize (layout::baseW, layout::baseH);
}

void MasterChainEditor::resized()
{
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) layout::baseW));
}
