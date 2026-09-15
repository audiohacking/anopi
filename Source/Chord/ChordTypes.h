#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace anopi
{

enum class Tonality : uint8_t
{
    Major = 0,
    Minor
};

enum class ScaleId : uint8_t
{
    Major = 0,
    NaturalMinor,
    HarmonicMinor,
    MelodicMinor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Locrian,
    HarmonicMajor,
    Count
};

enum class ColorMode : uint8_t
{
    Diatonic = 0,
    Add6,
    SixNine,
    Quartal,
    Count
};

enum class LayoutMode : uint8_t
{
    Static = 0,
    RealScale
};

enum class ExtensionStage : uint8_t
{
    Fifths = 0,
    Triad,
    Seventh,
    Ninth,
    Eleventh,
    Thirteenth
};

struct ChordRequest
{
    int tonalCenter = 0;
    Tonality tonality = Tonality::Major;
    ScaleId scale = ScaleId::Major;
    ColorMode color = ColorMode::Diatonic;
    int inversion = 0;
    LayoutMode layout = LayoutMode::Static;
    int degreeKeyPc = 0;
    bool shift = false;
    float extensions = 0.4f;
    float voicing = 0.45f;
};

struct ChordResult
{
    std::vector<int> midiNotes;
    int rootPc = 0;
    int bassNote = -1;
    int altBassNote = -1;
    std::string symbol;
    std::string roman;
    std::string function;
    std::string layoutLabel;
    ExtensionStage stage = ExtensionStage::Triad;
    bool chromatic = false;
    bool shifted = false;
};

inline constexpr std::array<const char*, 12> kPcSharpNames = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

inline constexpr std::array<const char*, 12> kPcFlatNames = {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

inline constexpr std::array<const char*, 10> kScaleNames = {
    "Major", "Minor", "Harm. minor", "Mel. minor", "Dorian",
    "Phrygian", "Lydian", "Mixolydian", "Locrian", "Harm. major"
};

inline constexpr std::array<const char*, 4> kColorNames = {
    "Diatonic", "add6", "6/9", "Quartal"
};

inline int wrapPc (int pc) noexcept
{
    pc %= 12;
    if (pc < 0)
        pc += 12;
    return pc;
}

inline ScaleId effectiveScale (const ChordRequest& request) noexcept
{
    if (request.scale != ScaleId::Major)
        return request.scale;
    return request.tonality == Tonality::Minor ? ScaleId::NaturalMinor : ScaleId::Major;
}

inline bool scalePrefersFlats (ScaleId scale) noexcept
{
    switch (scale)
    {
        case ScaleId::NaturalMinor:
        case ScaleId::HarmonicMinor:
        case ScaleId::MelodicMinor:
        case ScaleId::Dorian:
        case ScaleId::Phrygian:
        case ScaleId::Locrian:
            return true;
        default:
            return false;
    }
}

inline ExtensionStage stageFromExtensions (float extensions) noexcept
{
    const float x = extensions < 0.0f ? 0.0f : (extensions > 1.0f ? 1.0f : extensions);
    if (x < 0.18f) return ExtensionStage::Fifths;
    if (x < 0.42f) return ExtensionStage::Triad;
    if (x < 0.58f) return ExtensionStage::Seventh;
    if (x < 0.72f) return ExtensionStage::Ninth;
    if (x < 0.86f) return ExtensionStage::Eleventh;
    return ExtensionStage::Thirteenth;
}

} // namespace anopi
