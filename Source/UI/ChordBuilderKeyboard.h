#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Chord/ChordTypes.h"
#include <array>
#include <bitset>
#include <functional>
#include <string>
#include <vector>

class ChordBuilderKeyboard : public juce::Component
{
public:
    std::function<void (int degree, bool on)> onDegree;

    void setHeld (std::bitset<13> held) noexcept { if (held != heldDegrees) { heldDegrees = held; repaint(); } }
    void setShifted (bool on) { shifted = on; }
    void setLabels (const std::array<std::string, 13>& next)
    {
        if (next != labels)
        {
            labels = next;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xffe8efe0));
        g.fillRoundedRectangle (r, 8.0f);

        const float w = r.getWidth();
        const float h = r.getHeight();
        const float whiteW = w / 8.0f;
        const int whitePcs[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };

        for (int i = 0; i < 8; ++i)
        {
            auto key = juce::Rectangle<float> (r.getX() + whiteW * (float) i + 2.0f, r.getY() + 8.0f,
                                               whiteW - 4.0f, h - 16.0f);
            const bool down = heldDegrees.test ((size_t) whitePcs[i]);
            g.setColour (down ? juce::Colour (0xffd8f0a8) : juce::Colours::white);
            g.fillRoundedRectangle (key, 4.0f);
            g.setColour (juce::Colour (0xff2a3328));
            g.drawRoundedRectangle (key, 4.0f, 1.2f);
            g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
            const auto& lab = labels[(size_t) whitePcs[i]];
            g.drawText (lab.empty() ? juce::String (whitePcs[i]) : juce::String (lab),
                        key.removeFromBottom (28.0f), juce::Justification::centred);
        }

        const int blackPcs[] = { 1, 3, 6, 8, 10 };
        const float blackX[] = { 0.7f, 1.7f, 3.7f, 4.7f, 5.7f };
        const float blackW = whiteW * 0.62f;
        const float blackH = h * 0.58f;

        for (int i = 0; i < 5; ++i)
        {
            auto key = juce::Rectangle<float> (r.getX() + whiteW * blackX[i], r.getY() + 8.0f, blackW, blackH);
            const bool down = heldDegrees.test ((size_t) blackPcs[i]);
            g.setColour (down ? juce::Colour (0xffe8a0c0) : juce::Colour (0xff2a3328));
            g.fillRoundedRectangle (key, 3.0f);
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (juce::String (labels[(size_t) blackPcs[i]]), key, juce::Justification::centredBottom);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override { hit (e, true); }
    void mouseUp (const juce::MouseEvent& e) override { hit (e, false); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const int d = degreeAt (e.position);
        if (d != dragDegree)
        {
            if (dragDegree >= 0 && onDegree)
                onDegree (dragDegree, false);
            dragDegree = d;
            if (dragDegree >= 0 && onDegree)
                onDegree (dragDegree, true);
        }
    }

private:
    std::bitset<13> heldDegrees;
    bool shifted = false;
    std::array<std::string, 13> labels {
        "I", "V/ii", "ii", "V/iii", "iii", "IV", "V/V", "V", "V/vi", "vi", "V/vii", "vii", "I"
    };
    int dragDegree = -1;

    int degreeAt (juce::Point<float> p) const
    {
        auto r = getLocalBounds().toFloat();
        const float whiteW = r.getWidth() / 8.0f;
        const int blackPcs[] = { 1, 3, 6, 8, 10 };
        const float blackX[] = { 0.7f, 1.7f, 3.7f, 4.7f, 5.7f };
        const float blackW = whiteW * 0.62f;
        const float blackH = r.getHeight() * 0.58f;
        for (int i = 0; i < 5; ++i)
        {
            auto key = juce::Rectangle<float> (r.getX() + whiteW * blackX[i], r.getY() + 8.0f, blackW, blackH);
            if (key.contains (p))
                return blackPcs[i];
        }
        const int whitePcs[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };
        int idx = juce::jlimit (0, 7, (int) ((p.x - r.getX()) / whiteW));
        return whitePcs[idx];
    }

    void hit (const juce::MouseEvent& e, bool on)
    {
        const int d = degreeAt (e.position);
        dragDegree = on ? d : -1;
        if (onDegree)
            onDegree (d, on);
    }
};
