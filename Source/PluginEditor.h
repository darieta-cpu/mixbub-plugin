#pragma once
#include "PluginProcessor.h"
#include "UI/Look.h"
#include "UI/Meters.h"
#include "UI/Controls.h"

namespace layout
{
    constexpr int baseW = 1450;
    constexpr int baseH = 470;
}

// Pannello con tutti i controlli, a dimensione fissa (baseW x baseH): l'editor lo scala
class MainPanel : public juce::Component, private juce::Timer
{
public:
    explicit MainPanel (MasterChainProcessor&);
    ~MainPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String&, juce::Colour);
    void setupToggle (juce::ToggleButton&, std::unique_ptr<ui::APVTS::ButtonAttachment>&, const char* id, juce::Colour);

    MasterChainProcessor& proc;
    ui::Look look;                       // dichiarato prima dei figli: distrutto per ultimo

    // Input
    ui::Knob gain;
    ui::LevelMeter inMeter;

    // Nastro
    juce::ToggleButton tapeOn;
    ui::Knob tapeDrive, tapeBump, tapeTone, tapeMix, tapeOut;
    ui::Combo tapeSpeed, tapePos;

    // Filtri
    juce::ToggleButton filtOn;
    ui::Knob hpf, lpf;
    ui::Combo hpfSlope, lpfSlope;

    // Pultec
    juce::ToggleButton eqOn;
    ui::Combo lfFreq, hfFreq, hfAtFreq;
    ui::Knob lfBoost, lfAtten, hfBoost, hfBw, hfAtten, sat;

    // Compressore
    juce::ToggleButton compOn;
    ui::Knob thr, makeup, mix;
    ui::Combo ratio, attack, release;
    ui::GRMeter compGr;

    // Limiter
    juce::ToggleButton limOn;
    ui::Knob limThr, limCeil, limRel;
    ui::GRMeter limGr;

    std::unique_ptr<ui::APVTS::ButtonAttachment> tapeAtt, filtAtt, eqAtt, compAtt, limAtt;
    juce::Rectangle<int> inputR, tapeR, filtR, eqR, compR, limR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};

class MasterChainEditor : public juce::AudioProcessorEditor
{
public:
    explicit MasterChainEditor (MasterChainProcessor&);
    void resized() override;

private:
    MainPanel panel;
};
