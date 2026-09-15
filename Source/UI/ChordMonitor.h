#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Chord/ChordTypes.h"

class ChordMonitor : public juce::Component
{
public:
    void setChord (const anopi::ChordResult& c)
    {
        symbol = c.symbol;
        roman = c.roman;
        function = c.function;
        layout = c.layoutLabel;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff1a1f18));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff8aa060));
        g.drawRoundedRectangle (r.reduced (0.8f), 10.0f, 1.2f);

        auto top = r.reduced (12.0f, 8.0f);
        g.setColour (juce::Colour (0xffd8f0a8));
        g.setFont (juce::FontOptions (28.0f).withStyle ("Bold"));
        g.drawText (symbol.isEmpty() ? "—" : symbol, top.removeFromTop (32.0f), juce::Justification::centredLeft);

        g.setFont (juce::FontOptions (16.0f));
        g.setColour (juce::Colour (0xffe8a0c0));
        g.drawText (roman, top.removeFromTop (22.0f), juce::Justification::centredLeft);

        g.setColour (juce::Colour (0xffa8c090));
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (function, top.removeFromTop (18.0f), juce::Justification::centredLeft);
        g.drawText (layout, top, juce::Justification::centredLeft);
    }

private:
    juce::String symbol, roman, function, layout;
};
