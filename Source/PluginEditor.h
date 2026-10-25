#pragma once
#include "PluginProcessor.h"

namespace ui
{
    inline const juce::Colour bg     { 0xff090a0f };
    inline const juce::Colour panel  { 0xff13151e };
    inline const juce::Colour line   { 0xff262a3a };
    inline const juce::Colour text   { 0xffdfe2f2 };
    inline const juce::Colour dim    { 0xff6d728c };
    inline const juce::Colour cold   { 0xff5b7cff };
    inline const juce::Colour warm   { 0xffffa95a };
    inline const juce::Colour violet { 0xffa28bff };
    inline const juce::Colour live   { 0xff4ade80 };

    inline juce::Font font (float size, bool bold = true)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }
}

//==============================================================================
class VibeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VibeLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, ui::panel);
        setColour (juce::ComboBox::outlineColourId, ui::line);
        setColour (juce::ComboBox::textColourId, ui::text);
        setColour (juce::ComboBox::arrowColourId, ui::dim);
        setColour (juce::PopupMenu::backgroundColourId, ui::panel);
        setColour (juce::PopupMenu::textColourId, ui::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::line);
    }

    juce::Colour vibeColour (float t) const { return ui::cold.interpolatedWith (ui::warm, t); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (16.0f);
        const float radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const auto col = vibeColour (pos);

        juce::ColourGradient glow (col.withAlpha (0.25f), c, col.withAlpha (0.0f),
                                   c.translated (radius * 1.45f, 0.0f), true);
        g.setGradientFill (glow);
        g.fillEllipse (b.expanded (radius * 0.45f));

        g.setColour (ui::panel);
        g.fillEllipse (b.reduced (radius * 0.14f));
        g.setColour (ui::line);
        g.drawEllipse (b.reduced (radius * 0.14f), 1.5f);

        const float arcR = radius - 4.0f;
        juce::Path track, val;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        val.addCentredArc   (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        const juce::PathStrokeType st (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        g.setColour (ui::line); g.strokePath (track, st);
        g.setColour (col);      g.strokePath (val, st);

        juce::Path p;
        p.addRoundedRectangle (-2.0f, -(arcR - 12.0f), 4.0f, radius * 0.30f, 2.0f);
        g.setColour (ui::text);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                           float, float, juce::Slider::SliderStyle, juce::Slider&) override
    {
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y + 6.0f, bottom = (float) (y + h) - 6.0f;
        g.setColour (ui::line);
        g.fillRoundedRectangle (cx - 2.0f, top, 4.0f, bottom - top, 2.0f);
        g.setColour (ui::violet);
        g.fillRoundedRectangle (cx - 2.0f, sliderPos, 4.0f, bottom - sliderPos, 2.0f);
        auto thumb = juce::Rectangle<float> (28.0f, 12.0f).withCentre ({ cx, sliderPos });
        g.setColour (ui::text);   g.fillRoundedRectangle (thumb, 6.0f);
        g.setColour (ui::violet); g.drawRoundedRectangle (thumb, 6.0f, 1.5f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        auto r = b.getLocalBounds().toFloat();
        auto dot = r.removeFromLeft (22.0f).withSizeKeepingCentre (14.0f, 14.0f);
        g.setColour (ui::line);
        g.drawEllipse (dot, 1.5f);
        if (b.getToggleState()) { g.setColour (ui::violet); g.fillEllipse (dot.reduced (3.0f)); }
        g.setColour (over ? ui::text : ui::dim);
        g.setFont (ui::font (11.0f));
        g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (6.0f).toNearestInt(),
                    juce::Justification::centredLeft);
    }
};

//==============================================================================
class SparkButton : public juce::Button
{
public:
    SparkButton() : juce::Button ("SPARK") { setTriggeredOnMouseDown (true); }

    void flash() { glow = 1.0f; repaint(); }
    void tick()  { if (glow > 0.01f) { glow *= 0.90f; repaint(); } else glow = 0.0f; }
    void setAccent (juce::Colour c) { if (c != accent) { accent = c; repaint(); } }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto area = getLocalBounds().toFloat();
        const float d = juce::jmin (area.getWidth(), area.getHeight());
        auto disc = area.withSizeKeepingCentre (d * 0.78f, d * 0.78f);
        if (down) disc = disc.reduced (d * 0.015f);
        const auto c = disc.getCentre();

        juce::ColourGradient halo (accent.withAlpha (0.18f + 0.55f * glow + (over ? 0.08f : 0.0f)), c,
                                   accent.withAlpha (0.0f), c.translated (d * 0.5f, 0.0f), true);
        g.setGradientFill (halo);
        g.fillEllipse (area);

        juce::ColourGradient body (juce::Colour (0xff1e2132), disc.getX(), disc.getY(),
                                   juce::Colour (0xff0d0e16), disc.getX(), disc.getBottom(), false);
        g.setGradientFill (body);
        g.fillEllipse (disc);
        g.setColour (accent.withAlpha (0.55f + 0.45f * glow));
        g.drawEllipse (disc, 2.5f);

        // lightning bolt
        const float s = disc.getWidth() * 0.075f;
        const float cx = c.x, cy = c.y - disc.getHeight() * 0.09f;
        juce::Path bolt;
        bolt.startNewSubPath (cx + 0.5f * s, cy - 2.0f * s);
        bolt.lineTo (cx - 1.0f * s, cy + 0.2f * s);
        bolt.lineTo (cx - 0.1f * s, cy + 0.2f * s);
        bolt.lineTo (cx - 0.5f * s, cy + 2.0f * s);
        bolt.lineTo (cx + 1.0f * s, cy - 0.4f * s);
        bolt.lineTo (cx + 0.1f * s, cy - 0.4f * s);
        bolt.closeSubPath();
        g.setColour (accent.brighter (0.3f + 0.4f * glow));
        g.fillPath (bolt);

        g.setColour (ui::text);
        g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold).withKerningFactor (0.3f)));
        g.drawText ("SPARK", disc.withTrimmedTop (disc.getHeight() * 0.64f).toNearestInt(),
                    juce::Justification::centredTop);
    }

private:
    float glow = 0.0f;
    juce::Colour accent { ui::violet };
};

//==============================================================================
class ChordStrip : public juce::Component
{
public:
    void setChords (const juce::StringArray& names, const juce::String& t)
    {
        chords = names; title = t; repaint();
    }
    void setActive (int i) { if (i != active) { active = i; repaint(); } }
    void setAccent (juce::Colour c) { if (c != accent) { accent = c; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (ui::dim);
        g.setFont (ui::font (10.5f));
        g.drawText (title.isEmpty() ? juce::String ("PRESS SPARK") : title.toUpperCase(),
                    r.removeFromTop (18.0f).toNearestInt(), juce::Justification::centredLeft);
        r.removeFromTop (4.0f);

        const float gap = 10.0f;
        const float w = (r.getWidth() - gap * 3.0f) / 4.0f;
        for (int i = 0; i < 4; ++i)
        {
            auto tile = juce::Rectangle<float> (r.getX() + (float) i * (w + gap), r.getY(), w, r.getHeight());
            const bool act = (i == active);
            g.setColour (act ? accent.withAlpha (0.18f) : ui::panel);
            g.fillRoundedRectangle (tile, 10.0f);
            g.setColour (act ? accent : ui::line);
            g.drawRoundedRectangle (tile.reduced (0.5f), 10.0f, act ? 2.0f : 1.0f);

            g.setColour (act ? juce::Colours::white : ui::text);
            g.setFont (ui::font (22.0f));
            g.drawText (i < chords.size() ? chords[i] : juce::String ("-"),
                        tile.toNearestInt(), juce::Justification::centred);
            g.setColour (ui::dim);
            g.setFont (ui::font (9.5f));
            g.drawText ("BAR " + juce::String (i + 1), tile.reduced (10.0f, 6.0f).toNearestInt(),
                        juce::Justification::topLeft);
        }
    }

private:
    juce::StringArray chords;
    juce::String title;
    int active = -1;
    juce::Colour accent { ui::violet };
};

//==============================================================================
class VibeSparkAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VibeSparkAudioProcessorEditor (VibeSparkAudioProcessor&);
    ~VibeSparkAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VibeSparkAudioProcessor& proc;
    VibeLookAndFeel laf;

    juce::Slider vibeSlider, spaceSlider;
    SparkButton sparkButton;
    ChordStrip chordStrip;
    juce::ComboBox keyBox;
    juce::ToggleButton loopButton { "Loop" }, portButton { "Virtual MIDI port" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> vibeAtt, spaceAtt;
    std::unique_ptr<CA> keyAtt;
    std::unique_ptr<BA> loopAtt, portAtt;

    juce::Rectangle<int> titleArea, bpmArea, knobArea, spaceArea, sparkArea, footerArea;
    double bpm = 120.0;
    bool playing = false;
    float beat = 0.0f;
    juce::Colour accent { ui::violet };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeSparkAudioProcessorEditor)
};
