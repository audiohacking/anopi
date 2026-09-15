#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

namespace anopi
{

class PreviewSynth
{
public:
    void prepare (double newSampleRate, int) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        inverseSr = 1.0f / (float) sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& bus : voices)
            for (auto& v : bus)
                v = {};
        bendRatio = 1.0f;
    }

    void handleMidi (const juce::MidiBuffer& midi)
    {
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            const int module = juce::jlimit (0, 3, msg.getChannel() - 1);

            if (msg.isNoteOn())
                noteOn (module, msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff())
                noteOff (module, msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allOff (module);
            else if (msg.isPitchWheel())
            {
                const float norm = ((float) msg.getPitchWheelValue() - 8192.0f) / 8192.0f;
                bendRatio = std::pow (2.0f, norm * 2.0f / 12.0f); // ±2 semitones
            }
        }
    }

    void render (juce::AudioBuffer<float>& buffer, float gain) noexcept
    {
        const int n = buffer.getNumSamples();
        const int nc = buffer.getNumChannels();
        buffer.clear();
        if (gain <= 0.0001f)
            return;

        auto* L = buffer.getWritePointer (0);
        auto* R = nc > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < n; ++i)
        {
            float mix = 0.0f;
            mix += renderModule (0, 0.55f, 0.22f, 0.008f, 0.28f, 0.62f, 0.22f, 0.42f); // keys
            mix += renderModule (1, 0.70f, 0.08f, 0.005f, 0.18f, 0.95f, 0.10f, 0.55f); // bass
            mix += renderModule (2, 0.48f, 0.18f, 0.002f, 0.10f, 0.12f, 0.08f, 0.85f); // arp
            mix += renderModule (3, 0.38f, 0.32f, 0.10f, 0.40f, 0.78f, 0.55f, 0.18f);  // pad

            mix = std::tanh (mix * 1.15f) * gain;
            L[i] = mix;
            if (R != nullptr)
                R[i] = mix;
        }
    }

private:
    struct Voice
    {
        bool active = false;
        int note = 0;
        float vel = 0;
        float phase = 0;
        float phase2 = 0;
        float env = 0;
        float cutoff = 0;
        uint32_t age = 0;
        bool releasing = false;
    };

    static constexpr int kVoices = 10;
    std::array<std::array<Voice, kVoices>, 4> voices {};
    double sampleRate = 44100.0;
    float inverseSr = 1.0f / 44100.0f;
    float bendRatio = 1.0f;
    uint32_t ageClock = 1;

    static float saw (float p) noexcept { return 2.0f * p - 1.0f; }
    static float tri (float p) noexcept { return 4.0f * std::abs (p - 0.5f) - 1.0f; }
    static float pulse (float p) noexcept { return p < 0.28f ? 1.0f : -1.0f; }
    static float sine (float p) noexcept { return std::sin (p * juce::MathConstants<float>::twoPi); }

    Voice* findVoice (int module, int note) noexcept
    {
        for (auto& v : voices[static_cast<size_t> (module)])
            if (v.active && v.note == note)
                return &v;
        return nullptr;
    }

    Voice* steal (int module) noexcept
    {
        Voice* best = &voices[static_cast<size_t> (module)][0];
        for (auto& v : voices[static_cast<size_t> (module)])
        {
            if (! v.active)
                return &v;
            if (v.age < best->age)
                best = &v;
        }
        return best;
    }

    void noteOn (int module, int note, float vel) noexcept
    {
        if (auto* existing = findVoice (module, note))
            existing->active = false;

        auto* v = steal (module);
        v->active = true;
        v->releasing = false;
        v->note = note;
        v->vel = juce::jlimit (0.05f, 1.0f, vel);
        v->phase = 0;
        v->phase2 = 0.02f;
        v->env = 0;
        v->cutoff = 0;
        v->age = ageClock++;
    }

    void noteOff (int module, int note) noexcept
    {
        if (auto* v = findVoice (module, note))
            v->releasing = true;
    }

    void allOff (int module) noexcept
    {
        for (auto& v : voices[static_cast<size_t> (module)])
            v.releasing = true;
    }

    float osc (int module, Voice& v) noexcept
    {
        switch (module)
        {
            case 1: return 0.88f * sine (v.phase) + 0.12f * saw (v.phase);                 // bass
            case 2: return 0.62f * pulse (v.phase) + 0.38f * sine (v.phase);                // arp
            case 3: return 0.55f * tri (v.phase) + 0.45f * tri (v.phase2);                  // pad
            default: return 0.48f * saw (v.phase) + 0.40f * sine (v.phase) + 0.12f * tri (v.phase2); // keys
        }
    }

    float renderModule (int module, float gain, float detune,
                        float attack, float decay, float sustain, float release,
                        float brightness) noexcept
    {
        float sum = 0.0f;
        const float aInc = inverseSr / std::max (attack, 0.001f);
        const float dInc = inverseSr / std::max (decay, 0.01f);
        const float rInc = inverseSr / std::max (release, 0.01f);
        const float lp = std::exp (-2.0f * juce::MathConstants<float>::pi * (400.0f + brightness * 2400.0f) * inverseSr);

        for (auto& v : voices[static_cast<size_t> (module)])
        {
            if (! v.active)
                continue;

            if (v.releasing)
            {
                v.env -= rInc;
                if (v.env <= 0.0f)
                {
                    v = {};
                    continue;
                }
            }
            else if (v.env < 1.0f && v.env < sustain + 0.2f)
            {
                v.env += aInc;
                if (v.env > 1.0f)
                    v.env = 1.0f;
            }
            else
            {
                if (v.env > sustain)
                    v.env -= dInc;
                if (v.env < sustain)
                    v.env = sustain;
            }

            const float freq = 440.0f * std::pow (2.0f, ((float) v.note - 69.0f) / 12.0f) * bendRatio;
            const float inc = freq * inverseSr;
            v.phase += inc;
            v.phase2 += inc * (1.0f + detune * 0.012f);
            if (v.phase >= 1.0f) v.phase -= 1.0f;
            if (v.phase2 >= 1.0f) v.phase2 -= 1.0f;

            float s = osc (module, v);
            v.cutoff += (1.0f - lp) * (s - v.cutoff);
            sum += v.cutoff * v.env * v.vel;
        }

        return sum * gain;
    }
};

} // namespace anopi
