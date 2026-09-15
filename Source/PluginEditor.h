#pragma once

#include "PluginProcessor.h"
#include "UI/ChordBuilderKeyboard.h"
#include "UI/ChordMonitor.h"
#include "UI/PadBank.h"
#include "UI/StrumStrip.h"
#include "UI/TonalSelector.h"
#include "Chord/ScaleTables.h"

#include <bitset>

class AnopiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AnopiLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId, juce::Colour (0xff2a3328));
        setColour (juce::Slider::trackColourId, juce::Colour (0xff8aa060));
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xffc5d0b8));
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xffd2dcc8));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff2a3328));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1c2418));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffeef4b4));
        setColour (juce::Label::textColourId, juce::Colour (0xff2a3328));
        setColour (juce::ToggleButton::textColourId, juce::Colour (0xff2a3328));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool highlighted, bool down) override
    {
        juce::ignoreUnused (highlighted);
        paintAnopiPad (g, button.getLocalBounds().toFloat().reduced (1.0f),
                       button.getToggleState() || down,
                       button.getButtonText(),
                       juce::jlimit (9.0f, 12.0f, (float) button.getHeight() * 0.40f));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        juce::ignoreUnused (highlighted);
        auto r = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = down || button.getToggleState();
        g.setColour (on ? juce::Colour (0xffeef4b4) : juce::Colour (0xffd2dcc8));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (on ? juce::Colour (0xff2a3328) : juce::Colour (0xff7e8b74));
        g.drawRoundedRectangle (r, 4.0f, on ? 1.7f : 1.0f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float, const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        g.setColour (juce::Colour (0xffc5d0b8));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colour (0xff8aa060));
        if (style == juce::Slider::LinearVertical)
        {
            auto fill = r.withTop (sliderPos);
            g.fillRoundedRectangle (fill, 3.0f);
            auto thumb = juce::Rectangle<float> (r.getX(), sliderPos - 5.0f, r.getWidth(), 10.0f);
            g.setColour (juce::Colour (0xff2a3328));
            g.fillRoundedRectangle (thumb, 2.0f);
        }
        else
        {
            auto fill = r.withWidth (juce::jmax (0.0f, sliderPos - r.getX()));
            g.fillRoundedRectangle (fill, 3.0f);
            auto thumb = juce::Rectangle<float> (sliderPos - 5.0f, r.getY(), 10.0f, r.getHeight());
            g.setColour (juce::Colour (0xff2a3328));
            g.fillRoundedRectangle (thumb, 2.0f);
        }
        juce::ignoreUnused (slider);
    }
};

class AnopiAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit AnopiAudioProcessorEditor (AnopiAudioProcessor&);
    ~AnopiAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void pushDegree (int degree, bool on);
    void setChoice (const char* paramId, int index);
    void syncQwertyDegrees();

    AnopiAudioProcessor& proc;
    AnopiLookAndFeel look;

    ChordMonitor monitor;
    ChordBuilderKeyboard keyboard;
    TonalSelector tonal;
    StrumStrip strum;
    PadBank scalePads { "SCALE", std::vector<PadItem> {
        { "Phr", 5 }, { "Min", 1 }, { "Dor", 4 }, { "Mix", 7 }, { "Maj", 0 },
        { "Loc", 8 }, { "H.min", 2 }, { "Mel", 3 }, { "H.maj", 9 }, { "Lyd", 6 }
    }, 5 };
    PadBank colorPads { "COLOR", { "3rd", "add6", "6/9", "4ths" }, 2 };
    PadBank invPads { "INV", { "R", "1", "2", "3" }, 2 };
    PadBank arpPads { "ARP", { "1/4", "1/8", "1/16", "1/32" }, 4 };
    PadBank outPads { "OUT", { "Ch 1-4", "Cables" }, 2 };
    PadBank octPads { "OCT", { "1", "2", "3", "4", "5", "6" }, 6 };
    PadBank extPads { "EXT", { "5", "3", "7", "9", "11", "13" }, 3 };

    juce::Slider voicing { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Slider bend { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Slider gain { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };

    juce::ToggleButton staticReal { "Real scale" };
    juce::ToggleButton shift { "Shift" };
    juce::ToggleButton sustain { "Sustain" };
    juce::ToggleButton keysOn { "Keys" };
    juce::ToggleButton bassOn { "Bass" };
    juce::ToggleButton arpOn { "Arp" };
    juce::ToggleButton padOn { "Pad" };
    juce::ToggleButton bassLink { "Bass link" };
    juce::ToggleButton padLatch { "Pad latch" };
    juce::ToggleButton preview { "Internal tones" };
    juce::ToggleButton capture { "Capture" };
    juce::ToggleButton qwertyHelp { "Show map" };

    juce::TextButton dumpBtn { "Dump MIDI" }, allOffBtn { "Panic" }, learnBtn { "MIDI learn" };

    juce::Label voiceLabel { {}, "VOICING" };
    juce::Label bendLabel { {}, "BEND" };
    juce::Label footer { {}, "Arrows move SCALE (left darker · right brighter · up/down parallel)" };

    using FAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<FAttach> voiceAt, gainAt;
    std::unique_ptr<BAttach> layAt, shiftAt, susAt, kAt, bAt, aAt, pAt, linkAt, latchAt, prevAt, capAt;

    std::bitset<13> qwertyHeld;
    std::bitset<4> qwertyStrum;
    bool qwertyBassRoot = false, qwertyBassAlt = false;
    bool showMap = false;
    int lastBend = 8192;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnopiAudioProcessorEditor)
};
