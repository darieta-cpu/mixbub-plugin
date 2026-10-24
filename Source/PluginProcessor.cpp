#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;

juce::ParameterID pid (const char* s) { return { s, 1 }; }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return { lo, hi,
             [] (float s, float e, float v) { return s * std::pow (e / s, v); },
             [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
             [] (float, float, float v) { return v; } };
}

std::unique_ptr<APF> fp (const char* id, const char* name, float lo, float hi, float def,
                         const char* unit = "", float step = 0.0f)
{
    const int decimals = step >= 1.0f ? 0 : (step >= 0.1f ? 1 : 2);
    const juce::String u (unit);
    auto attr = juce::AudioParameterFloatAttributes()
                    .withLabel (u)
                    .withStringFromValueFunction ([u, decimals] (float v, int)
                    {
                        return juce::String (v, decimals) + (u.isEmpty() ? juce::String() : " " + u);
                    })
                    .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
    return std::make_unique<APF> (pid (id), name, juce::NormalisableRange<float> (lo, hi, step), def, attr);
}

std::unique_ptr<APF> fpLog (const char* id, const char* name, float lo, float hi, float def, const char* unit)
{
    const juce::String u (unit);
    auto attr = juce::AudioParameterFloatAttributes()
                    .withLabel (u)
                    .withStringFromValueFunction ([u] (float v, int)
                    {
                        if (u == "Hz")
                            return v >= 1000.0f ? juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz"
                                                : juce::String (juce::roundToInt (v)) + " Hz";
                        return juce::String (juce::roundToInt (v)) + " " + u;
                    })
                    .withValueFromStringFunction ([] (const juce::String& t)
                    {
                        const auto s = t.trim().toLowerCase();
                        return s.contains ("k") ? s.getFloatValue() * 1000.0f : s.getFloatValue();
                    });
    return std::make_unique<APF> (pid (id), name, logRange (lo, hi), def, attr);
}

std::unique_ptr<APC> cp (const char* id, const char* name, juce::StringArray items, int def)
{
    return std::make_unique<APC> (pid (id), name, items, def);
}

std::unique_ptr<APB> bp (const char* id, const char* name, bool def)
{
    return std::make_unique<APB> (pid (id), name, def);
}

constexpr float kRatios[]   { 2.0f, 4.0f, 10.0f };
constexpr float kAttacksMs[] { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };
constexpr float kReleasesMs[] { 100.0f, 300.0f, 600.0f, 1200.0f };
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout MasterChainProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (fp ("in_gain", "Input Gain", -12.0f, 12.0f, 0.0f, "dB", 0.1f));

    // Filtri
    l.add (bp ("filt_on", "Filters On", true));
    l.add (fpLog ("hpf_freq", "HPF Freq", 20.0f, 20000.0f, 20.0f, "Hz"));
    l.add (cp ("hpf_slope", "HPF Slope", { "12 dB/oct", "24 dB/oct" }, 0));
    l.add (fpLog ("lpf_freq", "LPF Freq", 20.0f, 20000.0f, 20000.0f, "Hz"));
    l.add (cp ("lpf_slope", "LPF Slope", { "12 dB/oct", "24 dB/oct" }, 0));

    // Pultec
    l.add (bp ("eq_on", "EQ On", true));
    l.add (cp ("eq_lf_freq", "EQ LF Freq", { "20 Hz", "30 Hz", "60 Hz", "100 Hz" }, 2));
    l.add (fp ("eq_lf_boost", "EQ LF Boost", 0.0f, 10.0f, 0.0f, "", 0.1f));
    l.add (fp ("eq_lf_atten", "EQ LF Atten", 0.0f, 10.0f, 0.0f, "", 0.1f));
    l.add (cp ("eq_hf_freq", "EQ HF Boost Freq", { "3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz" }, 4));
    l.add (fp ("eq_hf_boost", "EQ HF Boost", 0.0f, 10.0f, 0.0f, "", 0.1f));
    l.add (fp ("eq_hf_bw", "EQ HF Bandwidth", 0.0f, 1.0f, 0.5f, "", 0.01f));
    l.add (cp ("eq_hf_atfreq", "EQ HF Atten Freq", { "5 kHz", "10 kHz", "20 kHz" }, 1));
    l.add (fp ("eq_hf_atten", "EQ HF Atten", 0.0f, 10.0f, 0.0f, "", 0.1f));
    l.add (fp ("eq_sat", "EQ Tube Sat", 0.0f, 1.0f, 0.0f, "", 0.01f));

    // Compressore
    l.add (bp ("comp_on", "Comp On", true));
    l.add (fp ("comp_thresh", "Comp Threshold", -30.0f, 0.0f, -12.0f, "dB", 0.1f));
    l.add (cp ("comp_ratio", "Comp Ratio", { "2:1", "4:1", "10:1" }, 0));
    l.add (cp ("comp_attack", "Comp Attack", { "0.1 ms", "0.3 ms", "1 ms", "3 ms", "10 ms", "30 ms" }, 4));
    l.add (cp ("comp_release", "Comp Release", { "0.1 s", "0.3 s", "0.6 s", "1.2 s", "Auto" }, 4));
    l.add (fp ("comp_makeup", "Comp Makeup", 0.0f, 20.0f, 0.0f, "dB", 0.1f));
    l.add (fp ("comp_mix", "Comp Mix", 0.0f, 100.0f, 100.0f, "%", 1.0f));

    // Limiter
    l.add (bp ("lim_on", "Limiter On", true));
    l.add (fp ("lim_thresh", "Limiter Threshold", -30.0f, 0.0f, 0.0f, "dB", 0.1f));
    l.add (fp ("lim_ceiling", "Limiter Ceiling", -6.0f, 0.0f, -0.3f, "dB", 0.1f));
    l.add (fpLog ("lim_release", "Limiter Release", 10.0f, 1000.0f, 100.0f, "ms"));

    return l;
}

MasterChainProcessor::MasterChainProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    p.inGain = g ("in_gain");
    p.filtOn = g ("filt_on");  p.hpfFreq = g ("hpf_freq");  p.hpfSlope = g ("hpf_slope");
    p.lpfFreq = g ("lpf_freq");  p.lpfSlope = g ("lpf_slope");
    p.eqOn = g ("eq_on");  p.eqLfFreq = g ("eq_lf_freq");  p.eqLfBoost = g ("eq_lf_boost");
    p.eqLfAtten = g ("eq_lf_atten");  p.eqHfFreq = g ("eq_hf_freq");  p.eqHfBoost = g ("eq_hf_boost");
    p.eqHfBw = g ("eq_hf_bw");  p.eqHfAtFreq = g ("eq_hf_atfreq");  p.eqHfAtten = g ("eq_hf_atten");
    p.eqSat = g ("eq_sat");
    p.compOn = g ("comp_on");  p.compThresh = g ("comp_thresh");  p.compRatio = g ("comp_ratio");
    p.compAttack = g ("comp_attack");  p.compRelease = g ("comp_release");
    p.compMakeup = g ("comp_makeup");  p.compMix = g ("comp_mix");
    p.limOn = g ("lim_on");  p.limThresh = g ("lim_thresh");  p.limCeiling = g ("lim_ceiling");
    p.limRelease = g ("lim_release");
}

bool MasterChainProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void MasterChainProcessor::prepareToPlay (double sampleRate, int)
{
    filters.prepare (sampleRate);
    pultec.prepare (sampleRate);
    compressor.prepare (sampleRate);
    limiter.prepare (sampleRate);
    setLatencySamples (limiter.getLatencySamples());   // il DAW compensa la latenza del lookahead
    lastInGain = (float) juce::Decibels::decibelsToGain (p.inGain->load());
}

void MasterChainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nCh = juce::jmin (2, buffer.getNumChannels());

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    float* const* ch = buffer.getArrayOfWritePointers();

    // Input gain (con rampa)
    const float gIn = (float) juce::Decibels::decibelsToGain (p.inGain->load());
    buffer.applyGainRamp (0, n, lastInGain, gIn);
    lastInGain = gIn;

    // Meter di ingresso (picco dopo il gain)
    for (int ch2 = 0; ch2 < nCh; ++ch2)
    {
        const float m = buffer.getMagnitude (ch2, 0, n);
        mc::atomicMax (ch2 == 0 ? inPeakL : inPeakR, m);
        if (nCh == 1) mc::atomicMax (inPeakR, m);
    }

    // 1. Filtri
    if (p.filtOn->load() > 0.5f)
    {
        filters.setParams (p.hpfFreq->load(), (int) p.hpfSlope->load(), p.lpfFreq->load(), (int) p.lpfSlope->load());
        filters.process (ch, nCh, n);
    }

    // 2. Pultec
    if (p.eqOn->load() > 0.5f)
    {
        mc::PultecEQ::Params e;
        e.lfFreqIdx = (int) p.eqLfFreq->load();   e.lfBoost = p.eqLfBoost->load();  e.lfAtten = p.eqLfAtten->load();
        e.hfFreqIdx = (int) p.eqHfFreq->load();   e.hfBoost = p.eqHfBoost->load();  e.hfBw = p.eqHfBw->load();
        e.hfAttenIdx = (int) p.eqHfAtFreq->load(); e.hfAtten = p.eqHfAtten->load(); e.sat = p.eqSat->load();
        pultec.setParams (e);
        pultec.process (ch, nCh, n);
    }

    // 3. Compressore bus
    if (p.compOn->load() > 0.5f)
    {
        const int rel = (int) p.compRelease->load();
        mc::BusCompressor::Params c;
        c.thresholdDb = p.compThresh->load();
        c.ratio       = kRatios[juce::jlimit (0, 2, (int) p.compRatio->load())];
        c.attackMs    = kAttacksMs[juce::jlimit (0, 5, (int) p.compAttack->load())];
        c.autoRelease = rel >= 4;
        c.releaseMs   = kReleasesMs[juce::jlimit (0, 3, rel)];
        c.makeupDb    = p.compMakeup->load();
        c.mix         = p.compMix->load() * 0.01f;
        compressor.setParams (c);
        compressor.process (ch, nCh, n);
    }

    // 4. Limiter (sempre in linea, anche in bypass, per mantenere costante la latenza)
    {
        mc::Limiter::Params l;
        l.thresholdDb = p.limThresh->load();
        l.ceilingDb   = p.limCeiling->load();
        l.releaseMs   = p.limRelease->load();
        l.bypass      = p.limOn->load() < 0.5f;
        limiter.setParams (l);
        limiter.process (ch, nCh, n);
    }
}

juce::AudioProcessorEditor* MasterChainProcessor::createEditor()
{
    return new MasterChainEditor (*this);
}

void MasterChainProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void MasterChainProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MasterChainProcessor();
}
