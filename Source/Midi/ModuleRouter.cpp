#include "ModuleRouter.h"

#include <algorithm>

namespace anopi
{

void ModuleRouter::prepare()
{
    for (auto& row : sounding)
        row.fill (0);
    for (auto& row : sustained)
        row.fill (0);
}

void ModuleRouter::setChannel (Module module, int channelOneBased) noexcept
{
    channels[static_cast<int> (module)] = juce::jlimit (1, 16, channelOneBased);
}

void ModuleRouter::setEnabled (Module module, bool on, juce::MidiBuffer& out, int sample)
{
    const int i = static_cast<int> (module);
    if (enabled[i] && ! on)
        allNotesOff (module, sample, out);
    enabled[i] = on;
}

void ModuleRouter::noteOn (Module module, int note, int velocity, int sample, juce::MidiBuffer& out)
{
    note = juce::jlimit (0, 127, note);
    velocity = juce::jlimit (1, 127, velocity);
    const int i = static_cast<int> (module);
    if (! enabled[i])
        return;

    if (sounding[i][static_cast<size_t> (note)] == 0)
        out.addEvent (juce::MidiMessage::noteOn (channels[i], note, (juce::uint8) velocity), sample);

    sounding[i][static_cast<size_t> (note)]++;
}

void ModuleRouter::noteOff (Module module, int note, int sample, juce::MidiBuffer& out)
{
    note = juce::jlimit (0, 127, note);
    const int i = static_cast<int> (module);
    auto& count = sounding[i][static_cast<size_t> (note)];
    if (count <= 0)
        return;

    if (sustain)
    {
        sustained[i][static_cast<size_t> (note)]++;
        return;
    }

    count--;
    if (count == 0 && enabled[i])
        out.addEvent (juce::MidiMessage::noteOff (channels[i], note), sample);
}

void ModuleRouter::allNotesOff (Module module, int sample, juce::MidiBuffer& out)
{
    const int i = static_cast<int> (module);
    for (int n = 0; n < 128; ++n)
    {
        if (sounding[i][static_cast<size_t> (n)] > 0 || sustained[i][static_cast<size_t> (n)] > 0)
            out.addEvent (juce::MidiMessage::noteOff (channels[i], n), sample);
        sounding[i][static_cast<size_t> (n)] = 0;
        sustained[i][static_cast<size_t> (n)] = 0;
    }
}

void ModuleRouter::allNotesOff (int sample, juce::MidiBuffer& out)
{
    for (int m = 0; m < 4; ++m)
        allNotesOff (static_cast<Module> (m), sample, out);
}

void ModuleRouter::pitchBend (int value14, int sample, juce::MidiBuffer& out)
{
    value14 = juce::jlimit (0, 16383, value14);
    for (int m = 0; m < 4; ++m)
        if (enabled[m])
            out.addEvent (juce::MidiMessage::pitchWheel (channels[m], value14), sample);
}

void ModuleRouter::setSustain (bool on, int sample, juce::MidiBuffer& out)
{
    if (sustain == on)
        return;

    sustain = on;
    if (on)
        return;

    for (int m = 0; m < 4; ++m)
    {
        for (int n = 0; n < 128; ++n)
        {
            auto& held = sustained[m][static_cast<size_t> (n)];
            if (held <= 0)
                continue;

            auto& count = sounding[m][static_cast<size_t> (n)];
            count = std::max (0, count - held);
            held = 0;
            if (count == 0 && enabled[m])
                out.addEvent (juce::MidiMessage::noteOff (channels[m], n), sample);
        }
    }
}

} // namespace anopi
