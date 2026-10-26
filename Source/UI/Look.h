#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui
{
namespace col
{
    inline const juce::Colour bg      { 0xff14161a };
    inline const juce::Colour panel   { 0xff1d2026 };
    inline const juce::Colour edge    { 0xff2c313a };
    inline const juce::Colour text    { 0xffd7dbe2 };
    inline const juce::Colour dim     { 0xff8a92a0 };
    inline const juce::Colour input   { 0xff5fcf8a };
    inline const juce::Colour tape    { 0xffd98c5f };
    inline const juce::Colour filters { 0xff4fb6c9 };
    inline const juce::Colour eq      { 0xffe0a43a };
    inline const juce::Colour comp    { 0xff6aa7ff };
    inline const juce::Colour limiter { 0xffe5584f };
}

class Look : public juce::LookAndFeel_V4
{
public:
    Look()
    {
        setColour (juce::Slider::textBoxTextColourId,       col::text);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,               col::dim);
        setColour (juce::ComboBox::backgroundColourId,      juce::Colour (0xff12141a));
        setColour (juce::ComboBox::textColourId,            col::text);
        setColour (juce::ComboBox::outlineColourId,         col::edge);
        setColour (juce::ComboBox::arrowColourId,           col::dim);
        setColour (juce::PopupMenu::backgroundColourId,     col::panel);
        setColour (juce::PopupMenu::textColourId,           col::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::edge);
        setColour (juce::PopupMenu::highlightedTextColourId, col::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
        const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const auto c = area.getCentre();
        const float thick = 3.5f;
        const float arcR = r - thick;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const juce::PathStrokeType stroke (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff2b3038));
        g.strokePath (track, stroke);

        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (val, stroke);

        const float br = arcR - 5.0f;
        g.setColour (juce::Colour (0xff2a2e36));
        g.fillEllipse (c.x - br, c.y - br, 2.0f * br, 2.0f * br);
        g.setColour (col::edge);
        g.drawEllipse (c.x - br, c.y - br, 2.0f * br, 2.0f * br, 1.0f);

        juce::Path p;
        p.addRoundedRectangle (-1.5f, -br + 3.0f, 3.0f, br * 0.5f, 1.0f);
        g.setColour (col::text);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();
        g.setColour (on ? b.findColour (juce::ToggleButton::tickColourId) : juce::Colour (0xff2b3038));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? juce::Colour (0xff101216) : col::dim);
        g.setFont (11.0f);
        g.drawText (b.getButtonText(), r, juce::Justification::centred, false);
    }
};
} // namespace ui
