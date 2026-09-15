#pragma once

#include "ChordTypes.h"

#include <optional>
#include <string>

namespace anopi
{

inline constexpr std::array<int, 7> kMajorScale = { 0, 2, 4, 5, 7, 9, 11 };
inline constexpr std::array<int, 7> kMinorScale = { 0, 2, 3, 5, 7, 8, 10 };

inline constexpr std::array<std::array<int, 7>, 10> kScaleTones = {{
    { 0, 2, 4, 5, 7,  9, 11 }, // Major
    { 0, 2, 3, 5, 7,  8, 10 }, // Natural minor
    { 0, 2, 3, 5, 7,  8, 11 }, // Harmonic minor
    { 0, 2, 3, 5, 7,  9, 11 }, // Melodic minor
    { 0, 2, 3, 5, 7,  9, 10 }, // Dorian
    { 0, 1, 3, 5, 7,  8, 10 }, // Phrygian
    { 0, 2, 4, 6, 7,  9, 11 }, // Lydian
    { 0, 2, 4, 5, 7,  9, 10 }, // Mixolydian
    { 0, 1, 3, 5, 6,  8, 10 }, // Locrian
    { 0, 2, 4, 5, 7,  8, 11 }  // Harmonic major
}};

inline constexpr std::array<const char*, 7> kDegreeFunctions = {
    "tonic", "supertonic", "mediant", "subdominant", "dominant", "submediant", "leading / subtonic"
};

inline constexpr std::array<const char*, 12> kMajorChromaticRoman = {
    "", "V/ii", "", "V/iii", "", "", "V/V", "", "V/vi", "", "V/vii", ""
};

inline constexpr std::array<const char*, 12> kMinorChromaticRoman = {
    "", "V/ii", "", "", "I/rel", "", "V/v", "", "", "IV", "", "V"
};

inline const std::array<int, 7>& scaleTones (ScaleId scale) noexcept
{
    const auto idx = static_cast<size_t> (scale);
    if (idx >= kScaleTones.size())
        return kScaleTones[0];
    return kScaleTones[idx];
}

inline const std::array<int, 7>& scaleFor (Tonality tonality) noexcept
{
    return tonality == Tonality::Major ? kMajorScale : kMinorScale;
}

inline std::optional<int> scaleDegreeIndex (int pc, ScaleId scale) noexcept
{
    pc = wrapPc (pc);
    const auto& tones = scaleTones (scale);
    for (int i = 0; i < 7; ++i)
        if (tones[static_cast<size_t> (i)] == pc)
            return i;
    return std::nullopt;
}

inline std::optional<int> scaleDegreeIndex (int pc, Tonality tonality) noexcept
{
    return scaleDegreeIndex (pc, tonality == Tonality::Minor ? ScaleId::NaturalMinor : ScaleId::Major);
}

inline bool isDiatonicPc (int pc, ScaleId scale) noexcept
{
    return scaleDegreeIndex (pc, scale).has_value();
}

inline int stackedInterval (ScaleId scale, int degreeIndex, int stack) noexcept
{
    const auto& tones = scaleTones (scale);
    const int from = tones[static_cast<size_t> ((degreeIndex % 7 + 7) % 7)];
    const int toIndex = degreeIndex + stack * 2;
    const int octaves = toIndex / 7;
    const int to = tones[static_cast<size_t> (toIndex % 7)] + 12 * octaves;
    int interval = to - from;
    while (interval < 0)
        interval += 12;
    return interval;
}

inline std::string diatonicRoman (ScaleId scale, int idx) noexcept
{
    idx = (idx % 7 + 7) % 7;
    static constexpr const char* upper[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    static constexpr const char* lower[] = { "i", "ii", "iii", "iv", "v", "vi", "vii" };

    const int pc = scaleTones (scale)[static_cast<size_t> (idx)];
    const int ref = kMajorScale[static_cast<size_t> (idx)];
    int diff = pc - ref;
    if (diff > 6) diff -= 12;
    if (diff < -6) diff += 12;

    std::string acc;
    if (diff == -1) acc = "b";
    else if (diff == 1) acc = "#";
    else if (diff <= -2) acc = "bb";

    const int third = stackedInterval (scale, idx, 1);
    const int fifth = stackedInterval (scale, idx, 2);
    const bool minorish = third <= 3;
    std::string out = acc + (minorish ? lower[idx] : upper[idx]);
    if (fifth == 6) out += "°";
    else if (fifth == 8) out += "+";
    return out;
}

inline std::array<std::string, 13> degreeLabels (ScaleId scale)
{
    std::array<std::string, 13> labels {};
    for (int pc = 0; pc < 13; ++pc)
    {
        const int wrapped = pc == 12 ? 0 : pc;
        if (auto idx = scaleDegreeIndex (wrapped, scale))
            labels[static_cast<size_t> (pc)] = diatonicRoman (scale, *idx);
        else
        {
            const auto& table = scalePrefersFlats (scale) ? kMinorChromaticRoman : kMajorChromaticRoman;
            const char* r = table[static_cast<size_t> (wrapped)];
            labels[static_cast<size_t> (pc)] = (r != nullptr && r[0] != '\0') ? r : "V/";
        }
    }
    return labels;
}

inline const char* pcName (int pc, bool preferFlats) noexcept
{
    pc = wrapPc (pc);
    return preferFlats ? kPcFlatNames[static_cast<size_t> (pc)]
                       : kPcSharpNames[static_cast<size_t> (pc)];
}

} // namespace anopi
