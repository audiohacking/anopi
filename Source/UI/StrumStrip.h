#pragma once

#include "PadBank.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>
#include <vector>

class StrumStrip : public juce::Component
{
public:
    std::function<void (int index, bool on)> onStrum;

    void setTones (const std::vector<int>& notes)
    {
        tones = notes;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xffe7eee0));
        g.fillRoundedRectangle (r, 6.0f);

        auto header = r.removeFromTop (16.0f);
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.setColour (juce::Colour (0xff5a6658));
        g.drawText ("STRUM", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft);

        const int n = 4;
        const float gap = 3.0f;
        const float w = (r.getWidth() - gap * (float) (n + 1)) / (float) n;
        const float h = r.getHeight() - gap * 2.0f;
        static const char* fallback[] = { "R", "3", "5", "7" };
        for (int i = 0; i < n; ++i)
        {
            auto cell = juce::Rectangle<float> (r.getX() + gap + (w + gap) * (float) i,
                                                r.getY() + gap, w, h);
            juce::String lab = fallback[i];
            if ((size_t) i < tones.size())
                lab = juce::MidiMessage::getMidiNoteName (tones[(size_t) i], true, true, 3);
            paintAnopiPad (g, cell, i == pressed, lab, juce::jlimit (10.0f, 13.0f, h * 0.42f));
        }
    }

    void mouseDown (const juce::MouseEvent& e) override { hit (e, true); }
    void mouseUp (const juce::MouseEvent& e) override { hit (e, false); }

private:
    std::vector<int> tones;
    int pressed = -1;

    void hit (const juce::MouseEvent& e, bool on)
    {
        auto r = getLocalBounds().toFloat();
        r.removeFromTop (16.0f);
        const int i = juce::jlimit (0, 3, (int) (4.0f * (e.position.x - r.getX()) / r.getWidth()));
        pressed = on ? i : -1;
        repaint();
        if (onStrum)
            onStrum (i, on);
    }
};
