#include "PluginEditor.h"

using namespace ui;

MasterChainEditor::MasterChainEditor (MasterChainProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      gain (p.apvts, "in_gain", "INPUT GAIN", col::input),
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
             &gain, &inMeter, &filtOn, &hpf, &lpf, &hpfSlope, &lpfSlope,
             &eqOn, &lfFreq, &hfFreq, &hfAtFreq, &lfBoost, &lfAtten, &hfBoost, &hfBw, &hfAtten, &sat,
             &compOn, &thr, &makeup, &mix, &ratio, &attack, &release, &compGr,
             &limOn, &limThr, &limCeil, &limRel, &limGr })
        addAndMakeVisible (c);

    setupToggle (filtOn, filtAtt, "filt_on", col::filters);
    setupToggle (eqOn,   eqAtt,   "eq_on",   col::eq);
    setupToggle (compOn, compAtt, "comp_on", col::comp);
    setupToggle (limOn,  limAtt,  "lim_on",  col::limiter);

    setSize (1180, 470);
    startTimerHz (30);
}

MasterChainEditor::~MasterChainEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MasterChainEditor::setupToggle (juce::ToggleButton& b, std::unique_ptr<APVTS::ButtonAttachment>& att,
                                     const char* id, juce::Colour accent)
{
    b.setButtonText ("ON");
    b.setColour (juce::ToggleButton::tickColourId, accent);
    att = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, id, b);
}

void MasterChainEditor::timerCallback()
{
    inMeter.push (proc.takeInputPeak (0), proc.takeInputPeak (1));
    compGr.push (proc.takeCompGainReductionDb());
    limGr.push (proc.takeLimiterGainReductionDb());
}

void MasterChainEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, juce::Colour accent)
{
    g.setColour (col::panel);
    g.fillRoundedRectangle (r.toFloat(), 8.0f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 8.0f, 1.0f);
    g.setColour (accent);
    g.setFont (13.0f);
    g.drawText (title, r.reduced (12, 10).removeFromTop (24), juce::Justification::centredLeft, false);
}

void MasterChainEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);

    g.setColour (col::text);
    g.setFont (g.getCurrentFont().boldened().withHeight (22.0f));
    g.drawText ("MASTERCHAIN", 18, 8, 300, 30, juce::Justification::centredLeft, false);
    g.setColour (col::dim);
    g.setFont (12.0f);
    g.drawText ("FILTERS  >  PULTEC EQ  >  BUS COMPRESSOR  >  LIMITER", 260, 8, 600, 30,
                juce::Justification::centredLeft, false);

    drawPanel (g, inputR, "INPUT",          col::input);
    drawPanel (g, filtR,  "FILTERS",        col::filters);
    drawPanel (g, eqR,    "PULTEC EQ",      col::eq);
    drawPanel (g, compR,  "BUS COMPRESSOR", col::comp);
    drawPanel (g, limR,   "LIMITER",        col::limiter);
}

void MasterChainEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (44);

    inputR = area.removeFromLeft (110);  area.removeFromLeft (10);
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
