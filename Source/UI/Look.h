#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui
{
namespace col
{
    inline const juce::Colour bg      { 0xff1c1d20 };
    inline const juce::Colour panel   { 0xff26272b };
    inline const juce::Colour edge    { 0xff6a6c72 };
    inline const juce::Colour text    { 0xffe6e6e6 };
    inline const juce::Colour dim     { 0xff9a9da5 };
    inline const juce::Colour accent  { 0xffff6a2b };   // striscia e firma
    inline const juce::Colour amber   { 0xffe8a317 };
    inline const juce::Colour ledOn   { 0xffff3b2a };
    inline const juce::Colour ledOff  { 0xff3a1210 };

    // Colori delle sezioni: nello stile micro rack la serigrafia e' tutta bianca
    // (i nomi restano per compatibilita' con PluginEditor.cpp)
    inline const juce::Colour input   { 0xfff0f0f0 };
    inline const juce::Colour tape    { 0xfff0f0f0 };
    inline const juce::Colour filters { 0xfff0f0f0 };
    inline const juce::Colour eq      { 0xfff0f0f0 };
    inline const juce::Colour comp    { 0xfff0f0f0 };
    inline const juce::Colour limiter { 0xfff0f0f0 };
}

// Stile "micro rack" anni '80: manopole nere con indice bianco, LED quadrati rossi, serigrafia bianca
class Look : public juce::LookAndFeel_V4
{
public:
    Look()
    {
        setColour (juce::Slider::textBoxTextColourId,       col::text);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,               col::text);
        setColour (juce::ComboBox::backgroundColourId,      juce::Colour (0xff0e0e10));
        setColour (juce::ComboBox::textColourId,            col::text);
        setColour (juce::ComboBox::outlineColourId,         col::edge);
        setColour (juce::ComboBox::arrowColourId,           col::text);
        setColour (juce::PopupMenu::backgroundColourId,     juce::Colour (0xff17181a));
        setColour (juce::PopupMenu::textColourId,           col::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff45464b));
        setColour (juce::PopupMenu::highlightedTextColourId, col::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto c = bounds.getCentre();
        const float R = d * 0.5f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        // scala serigrafata
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
            const float s = std::sin (a), co = std::cos (a);
            const bool major = (i % 5 == 0);
            const float r1 = R * 0.84f;
            const float r2 = R * (major ? 1.0f : 0.93f);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.drawLine (c.x + r1 * s, c.y - r1 * co, c.x + r2 * s, c.y - r2 * co, major ? 1.6f : 1.0f);
        }

        // corpo della manopola
        const float bodyR = R * 0.70f;
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - bodyR, c.y - bodyR + 2.0f, bodyR * 2.0f, bodyR * 2.0f);

        juce::ColourGradient grad (juce::Colour (0xff505053), c.x - bodyR * 0.5f, c.y - bodyR * 0.6f,
                                   juce::Colour (0xff0b0b0c), c.x + bodyR * 0.6f, c.y + bodyR * 0.7f, false);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (juce::Colour (0xff6c6e74));
        g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

        const float capR = bodyR * 0.72f;
        g.setColour (juce::Colour (0xff161618));
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);

        // indice bianco
        juce::Path pointer;
        pointer.addRoundedRectangle (-1.3f, -bodyR * 0.95f, 2.6f, bodyR * 0.75f, 1.0f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
        g.setColour (juce::Colour (0xfff4f4f4));
        g.fillPath (pointer);
    }

    // Pulsante con LED quadrato rosso
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();

        g.setColour (juce::Colour (highlighted ? 0xff46474c : 0xff3b3c40));
        g.fillRoundedRectangle (r, 3.0f);
        if (down)
        {
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.fillRoundedRectangle (r, 3.0f);
        }
        g.setColour (col::edge);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        juce::Rectangle<float> led (r.getX() + 5.0f, r.getCentreY() - 3.5f, 7.0f, 7.0f);
        if (on)
        {
            g.setColour (col::ledOn.withAlpha (0.35f));
            g.fillEllipse (led.expanded (3.0f));
            g.setColour (col::ledOn);
        }
        else
            g.setColour (col::ledOff);

        g.fillRect (led);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (led, 1.0f);

        g.setColour (col::text);
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (16.0f), juce::Justification::centred, false);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                       juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        juce::Path arrow;
        const float cx = (float) width - 12.0f, cy = (float) height * 0.5f;
        arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }
};
} // namespace ui
