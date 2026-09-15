#pragma once

#include "PadBank.h"
#include <functional>

class TonalSelector : public juce::Component
{
public:
    std::function<void (int pc)> onSelect;

    void setSelected (int pc)
    {
        pc = juce::jlimit (0, 11, pc);
        if (pc != selected)
        {
            selected = pc;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xffe7eee0));
        g.fillRoundedRectangle (r, 6.0f);

        auto header = r.removeFromTop (16.0f);
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.setColour (juce::Colour (0xff5a6658));
        g.drawText ("KEY", header.reduced (8.0f, 0.0f), juce::Justification::centredLeft);

        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const float gap = 3.0f;
        const float w = (r.getWidth() - gap * 7.0f) / 6.0f;
        const float h = (r.getHeight() - gap * 3.0f) / 2.0f;
        const float font = juce::jlimit (10.0f, 13.0f, h * 0.45f);

        for (int i = 0; i < 12; ++i)
        {
            const int row = i / 6;
            const int col = i % 6;
            auto cell = juce::Rectangle<float> (r.getX() + gap + (w + gap) * (float) col,
                                                r.getY() + gap + (h + gap) * (float) row,
                                                w, h);
            paintAnopiPad (g, cell, i == selected, names[i], font);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto r = getLocalBounds().toFloat();
        r.removeFromTop (16.0f);
        const float gap = 3.0f;
        const float w = (r.getWidth() - gap * 7.0f) / 6.0f;
        const float h = (r.getHeight() - gap * 3.0f) / 2.0f;
        for (int i = 0; i < 12; ++i)
        {
            const int row = i / 6;
            const int col = i % 6;
            auto cell = juce::Rectangle<float> (r.getX() + gap + (w + gap) * (float) col,
                                                r.getY() + gap + (h + gap) * (float) row,
                                                w, h);
            if (cell.contains (e.position))
            {
                selected = i;
                repaint();
                if (onSelect)
                    onSelect (i);
                return;
            }
        }
    }

private:
    int selected = 0;
};
