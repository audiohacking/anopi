#include "Voicing.h"
#include "ChordTypes.h"

namespace anopi
{

int bassRegisterNote (int rootPc, float voicing) noexcept
{
    rootPc = wrapPc (rootPc);
    const int octave = voicing < 0.7f ? 36 : 48;
    return octave + rootPc;
}

int altBassRegisterNote (int rootPc, int fifthOrThirdInterval, float voicing) noexcept
{
    const int bass = bassRegisterNote (rootPc, voicing);
    int alt = bass + fifthOrThirdInterval;

    while (alt > bass + 9)
        alt -= 12;
    while (alt < bass - 2)
        alt += 12;

    return alt;
}

std::vector<int> voiceChord (const std::vector<int>& relativeIntervals,
                             int rootPc,
                             float voicing,
                             int /*bassOctaveMidi*/)
{
    rootPc = wrapPc (rootPc);
    const int center = 52 + static_cast<int> (std::clamp (voicing, 0.0f, 1.0f) * 24.0f);

    std::vector<int> notes;
    notes.reserve (relativeIntervals.size());

    for (int interval : relativeIntervals)
    {
        int pc = wrapPc (rootPc + interval);
        int note = pc;

        while (note < center - 6)
            note += 12;
        while (note > center + 6)
            note -= 12;

        notes.push_back (std::clamp (note, 21, 108));
    }

    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    return notes;
}

std::vector<int> applyInversion (std::vector<int> notes, int inversion)
{
    if (notes.size() < 2)
        return notes;

    std::sort (notes.begin(), notes.end());
    const int n = (int) notes.size();
    inversion = ((inversion % n) + n) % n;

    for (int i = 0; i < inversion; ++i)
    {
        const int moved = notes.front() + 12;
        notes.erase (notes.begin());
        notes.push_back (moved);
    }

    std::sort (notes.begin(), notes.end());
    return notes;
}

} // namespace anopi
