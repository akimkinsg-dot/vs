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

class XYVibePad : public juce::Component
{
public:
    std::function<void(float, float)> onPadChanged;
    std::function<void()> onPadDragEnded;

    void setPosition (float x, float y)
    {
        padX = juce::jlimit (0.0f, 1.0f, x);
        padY = juce::jlimit (0.0f, 1.0f, y);
        repaint();
    }

    float getXVal() const { return padX; }
    float getYVal() const { return padY; }

    void mouseDown (const juce::MouseEvent& e) override { updateFromMouse (e); }
    void mouseDrag (const juce::MouseEvent& e) override { updateFromMouse (e); }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (onPadDragEnded) onPadDragEnded();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (ui::panel);
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (ui::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);

        juce::Colour currentAccent = ui::cold.interpolatedWith (ui::warm, padX);
        juce::ColourGradient grad (ui::cold.withAlpha (0.12f), 0.0f, r.getBottom(),
                                   ui::warm.withAlpha (0.12f), r.getRight(), 0.0f, false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 12.0f);

        g.setColour (ui::line.withAlpha (0.4f));
        g.drawHorizontalLine ((int) (r.getHeight() * 0.5f), 0.0f, r.getWidth());
        g.drawVerticalLine ((int) (r.getWidth() * 0.5f), 0.0f, r.getHeight());

        g.setFont (ui::font (11.0f));
        g.setColour (ui::cold);
        g.drawText ("2AM CONFESSION", r.reduced (14.0f), juce::Justification::bottomLeft);
        g.setColour (ui::warm);
        g.drawText ("LATE NIGHT DRIVE", r.reduced (14.0f), juce::Justification::bottomRight);
        g.setColour (ui::text.withAlpha (0.7f));
        g.drawText ("GOLDEN HOUR HEARTBREAK", r.reduced (14.0f), juce::Justification::topRight);
        g.drawText ("ETHEREAL VOID", r.reduced (14.0f), juce::Justification::topLeft);

        const float px = padX * r.getWidth();
        const float py = (1.0f - padY) * r.getHeight();
        auto dotArea = juce::Rectangle<float> (26.0f, 26.0f).withCentre ({ px, py });

        juce::ColourGradient halo (currentAccent.withAlpha (0.6f), px, py,
                                   currentAccent.withAlpha (0.0f), px + 35.0f, py, true);
        g.setGradientFill (halo);
        g.fillEllipse (dotArea.expanded (16.0f));

        g.setColour (ui::text);
        g.fillEllipse (dotArea);
        g.setColour (currentAccent);
        g.drawEllipse (dotArea, 2.5f);
    }

private:
    void updateFromMouse (const juce::MouseEvent& e)
    {
        auto r = getLocalBounds().toFloat();
        padX = juce::jlimit (0.0f, 1.0f, e.x / r.getWidth());
        padY = juce::jlimit (0.0f, 1.0f, 1.0f - (e.y / r.getHeight()));
        repaint();
        if (onPadChanged) onPadChanged (padX, padY);
    }

    float padX = 0.5f, padY = 0.5f;
};

class ChordStrip : public juce::Component
{
public:
    std::function<void()> onStartDragAll;
    std::function<void(int)> onStartDragSingle;
    std::function<void(int)> onClickPreviewChord;

    void setChords (const juce::StringArray& names, const juce::String& t)
    {
        chords = names; title = t; repaint();
    }
    void setActive (int i) { if (i != active) { active = i; repaint(); } }
    void setAccent (juce::Colour c) { if (c != accent) { accent = c; repaint(); } }

    void mouseDown (const juce::MouseEvent& e) override
    {
        int clickedIdx = getChordIndexAt (e.getMouseDownX());
        if (clickedIdx >= 0 && clickedIdx < chords.size())
        {
            if (onClickPreviewChord) onClickPreviewChord (clickedIdx);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        int clickedIdx = getChordIndexAt (e.getMouseDownX());
        if (clickedIdx >= 0 && clickedIdx < chords.size())
        {
            if (onStartDragSingle) onStartDragSingle (clickedIdx);
        }
        else
        {
            if (onStartDragAll) onStartDragAll();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (ui::dim);
        g.setFont (ui::font (10.5f));
        g.drawText (title.isEmpty() ? juce::String ("MOVE XY PAD TO VIBE") : title.toUpperCase() + "  (CLICK TO PREVIEW, DRAG TO EXPORT)",
                    r.removeFromTop (18.0f).toNearestInt(), juce::Justification::centredLeft);
        r.removeFromTop (4.0f);

        const float gap = 10.0f;
        const int numChords = std::max (1, chords.size());
        const float w = (r.getWidth() - gap * (float)(numChords - 1)) / (float)numChords;
        for (int i = 0; i < numChords; ++i)
        {
            auto tile = juce::Rectangle<float> (r.getX() + (float) i * (w + gap), r.getY(), w, r.getHeight());
            const bool act = (i == active);
            g.setColour (act ? accent.withAlpha (0.22f) : ui::panel);
            g.fillRoundedRectangle (tile, 10.0f);
            g.setColour (act ? accent : ui::line);
            g.drawRoundedRectangle (tile.reduced (0.5f), 10.0f, act ? 2.0f : 1.0f);

            g.setColour (act ? juce::Colours::white : ui::text);
            g.setFont (ui::font (20.0f));
            g.drawText (i < chords.size() ? chords[i] : juce::String ("-"),
                        tile.toNearestInt(), juce::Justification::centred);
            g.setColour (ui::dim);
            g.setFont (ui::font (9.0f));
            g.drawText ("BAR " + juce::String (i + 1), tile.reduced (8.0f, 6.0f).toNearestInt(),
                        juce::Justification::topLeft);
        }
    }

private:
    int getChordIndexAt (float x) const
    {
        auto r = getLocalBounds().toFloat().withTrimmedTop(22.0f);
        const float gap = 10.0f;
        const int numChords = std::max (1, chords.size());
        const float w = (r.getWidth() - gap * (float)(numChords - 1)) / (float)numChords;
        
        for (int i = 0; i < numChords; ++i)
        {
            float tileX = r.getX() + (float) i * (w + gap);
            if (x >= tileX && x <= tileX + w) return i;
        }
        return -1;
    }

    juce::StringArray chords;
    juce::String title;
    int active = -1;
    juce::Colour accent { ui::violet };
};

class VibeSparkAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VibeSparkAudioProcessorEditor (VibeSparkAudioProcessor&);
    ~VibeSparkAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void exportAndDragAllMidi();
    void exportAndDragSingleMidi (int index);
    void previewChordClicked (int index);

    VibeSparkAudioProcessor& proc;
    VibeLookAndFeel laf;

    XYVibePad vibePad;
    ChordStrip chordStrip;
    juce::ComboBox keyBox;
    juce::ToggleButton loopButton { "Loop" }, portButton { "Virtual MIDI port" };

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CA> keyAtt;
    std::unique_ptr<BA> loopAtt, portAtt;

    juce::Rectangle<int> titleArea, bpmArea, padArea, footerArea;
    double bpm = 120.0;
    bool playing = false;
    float beat = 0.0f;
    juce::Colour accent { ui::violet };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeSparkAudioProcessorEditor)
};
