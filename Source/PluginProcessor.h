#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Filters.h"
#include "DSP/PultecEQ.h"
#include "DSP/BusCompressor.h"
#include "DSP/Limiter.h"

class MasterChainProcessor : public juce::AudioProcessor
{
public:
    MasterChainProcessor();
    ~MasterChainProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MasterChain"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Moduli, in ordine di catena
    mc::FilterModule  filters;
    mc::PultecEQ      pultec;
    mc::BusCompressor compressor;
    mc::Limiter       limiter;

    float lastInGain = 1.0f;

    struct Ptrs
    {
        std::atomic<float> *inGain, *filtOn, *hpfFreq, *hpfSlope, *lpfFreq, *lpfSlope,
                           *eqOn, *eqLfFreq, *eqLfBoost, *eqLfAtten, *eqHfFreq, *eqHfBoost,
                           *eqHfBw, *eqHfAtFreq, *eqHfAtten, *eqSat,
                           *compOn, *compThresh, *compRatio, *compAttack, *compRelease,
                           *compMakeup, *compMix,
                           *limOn, *limThresh, *limCeiling, *limRelease;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterChainProcessor)
};
