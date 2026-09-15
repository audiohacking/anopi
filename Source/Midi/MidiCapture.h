#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cstdint>

namespace anopi
{

class MidiCapture
{
public:
    void prepare (double sr) noexcept { sampleRate = sr > 0.0 ? sr : 44100.0; }

    void setRecording (bool on) noexcept
    {
        if (on && ! recording.load())
        {
            for (auto& t : tracks)
                t.clear();
            sampleCounter = 0;
        }
        recording.store (on);
    }

    bool isRecording() const noexcept { return recording.load(); }

    void tap (const juce::MidiBuffer& buffer, int numSamples, const std::array<int, 4>& channels)
    {
        if (! recording.load())
        {
            sampleCounter += numSamples;
            return;
        }

        for (const auto metadata : buffer)
        {
            const auto msg = metadata.getMessage();
            if (! msg.isNoteOnOrOff() && ! msg.isPitchWheel())
                continue;

            const int ch = msg.getChannel();
            for (int i = 0; i < 4; ++i)
            {
                if (ch == channels[static_cast<size_t> (i)])
                {
                    auto stamped = msg;
                    tracks[static_cast<size_t> (i)].addEvent (stamped, sampleCounter + metadata.samplePosition);
                    break;
                }
            }
        }

        sampleCounter += numSamples;
    }

    juce::MidiFile toMidiFile (double bpm) const
    {
        juce::MidiFile file;
        file.setTicksPerQuarterNote (480);
        const double ticksPerSample = (bpm > 0.0 ? bpm : 120.0) / 60.0 * 480.0 / sampleRate;

        const char* names[] = { "ANOPI Keys", "ANOPI Bass", "ANOPI Arp", "ANOPI Pad" };

        for (int i = 0; i < 4; ++i)
        {
            juce::MidiMessageSequence seq;
            seq.addEvent (juce::MidiMessage::textMetaEvent (3, names[i]));
            seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60'000'000.0 / (bpm > 0.0 ? bpm : 120.0))));

            for (int e = 0; e < tracks[static_cast<size_t> (i)].getNumEvents(); ++e)
            {
                const auto* held = tracks[static_cast<size_t> (i)].getEventPointer (e);
                const double tick = held->message.getTimeStamp() * ticksPerSample;
                seq.addEvent (held->message, tick);
            }

            seq.updateMatchedPairs();
            file.addTrack (seq);
        }

        return file;
    }

    bool hasEvents() const noexcept
    {
        for (auto& t : tracks)
            if (t.getNumEvents() > 0)
                return true;
        return false;
    }

private:
    double sampleRate = 44100.0;
    int64_t sampleCounter = 0;
    std::atomic<bool> recording { false };
    std::array<juce::MidiMessageSequence, 4> tracks;
};

} // namespace anopi
