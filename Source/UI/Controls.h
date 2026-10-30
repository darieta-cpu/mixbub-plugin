#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Look.h"

namespace ui
{
using APVTS = juce::AudioProcessorValueTreeState;

// Manopola rotativa + nome sopra + valore sotto, collegata a un parametro
struct Knob : public juce::Component
{
    Knob (APVTS& apvts, const juce::String& id, const juce::String& name, juce::Colour accent)
    {
        label.setText (name.toUpperCase(), juce::dontSendNotification);
        label.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 16);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (label);
        addAndMakeVisible (slider);
        att = std::make_unique<APVTS::SliderAttachment> (apvts, id, slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (14));
        slider.setBounds (r);
    }

    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> att;   // dichiarato per ultimo: distrutto per primo
};

// Menu a tendina + nome sopra, collegato a un parametro "choice"
struct Combo : public juce::Component
{
    Combo (APVTS& apvts, const juce::String& id, const juce::String& name)
    {
        label.setText (name.toUpperCase(), juce::dontSendNotification);
        label.setFont (juce::FontOptions (10.5f).withStyle ("Bold"));
        label.setJustificationType (juce::Justification::centredLeft);
        label.setInterceptsMouseClicks (false, false);

        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
            box.addItemList (p->choices, 1);

        addAndMakeVisible (label);
        addAndMakeVisible (box);
        att = std::make_unique<APVTS::ComboBoxAttachment> (apvts, id, box);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (14));
        box.setBounds (r.removeFromTop (24));
    }

    juce::Label label;
    juce::ComboBox box;
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
};
} // namespace ui
