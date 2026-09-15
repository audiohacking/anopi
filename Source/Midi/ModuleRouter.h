#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>

namespace anopi
{

enum class Module : int
{
    Keys = 0,
    Bass,
    Arp,
    Pad,
    Count
};

class ModuleRouter
{
public:
    void prepare();
    void setEnabled (Module module, bool on, juce::MidiBuffer& out, int sample);
    bool isEnabled (Module module) const noexcept { return enabled[(size_t) static_cast<int> (module)]; }
    void setChannel (Module module, int channelOneBased) noexcept;
    int getChannel (Module module) const noexcept { return channels[(size_t) static_cast<int> (module)]; }

    void noteOn (Module module, int note, int velocity, int sample, juce::MidiBuffer& out);
    void noteOff (Module module, int note, int sample, juce::MidiBuffer& out);
    void allNotesOff (Module module, int sample, juce::MidiBuffer& out);
    void allNotesOff (int sample, juce::MidiBuffer& out);

    void pitchBend (int value14, int sample, juce::MidiBuffer& out);

    void setSustain (bool on, int sample, juce::MidiBuffer& out);
    bool getSustain() const noexcept { return sustain; }

private:
    std::array<int, 4> channels { 1, 2, 3, 4 };
    std::array<bool, 4> enabled { true, true, false, true };
    std::array<std::array<int, 128>, 4> sounding {};
    std::array<std::array<int, 128>, 4> sustained {};
    bool sustain = false;
};

} // namespace anopi
