#pragma once

#include <algorithm>
#include <vector>

namespace anopi
{

class ArpClock
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        samplesUntilNext = 0;
        index = 0;
    }

    void setChord (const std::vector<int>& notes)
    {
        chord = notes;
        if (index >= (int) chord.size())
            index = 0;
    }

    void reset() noexcept
    {
        index = 0;
        samplesUntilNext = 0;
        lastNote = -1;
    }

    template <typename EmitOn, typename EmitOff>
    void process (int numSamples, double bpm, int division, bool enabled,
                  EmitOn&& noteOn, EmitOff&& noteOff)
    {
        if (! enabled || chord.empty() || bpm <= 0.0)
        {
            if (lastNote >= 0)
            {
                noteOff (lastNote, 0);
                lastNote = -1;
            }
            return;
        }

        const double beatsPerSec = bpm / 60.0;
        const double stepsPerBeat = (double) division / 4.0; // 4=1/4, 8=1/8, 16=1/16
        const int stepSamples = std::max (1, (int) (sampleRate / (beatsPerSec * stepsPerBeat)));

        int sample = 0;
        while (sample < numSamples)
        {
            if (samplesUntilNext <= 0)
            {
                if (lastNote >= 0)
                    noteOff (lastNote, sample);

                lastNote = chord[static_cast<size_t> (index % (int) chord.size())];
                noteOn (lastNote, sample);
                index = (index + 1) % std::max (1, (int) chord.size());
                samplesUntilNext += stepSamples;
            }

            const int run = std::min (numSamples - sample, samplesUntilNext);
            samplesUntilNext -= run;
            sample += run;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<int> chord;
    int index = 0;
    int samplesUntilNext = 0;
    int lastNote = -1;
};

} // namespace anopi
