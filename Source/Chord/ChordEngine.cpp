#include "ChordEngine.h"
#include "ScaleTables.h"
#include "Voicing.h"

#include <algorithm>
#include <sstream>

namespace anopi
{
namespace
{

bool hasInterval (const std::vector<int>& intervals, int value) noexcept
{
    return std::find (intervals.begin(), intervals.end(), value) != intervals.end();
}

void addUnique (std::vector<int>& intervals, int value)
{
    if (! hasInterval (intervals, value))
        intervals.push_back (value);
}

std::string qualitySuffix (const std::vector<int>& intervals)
{
    const bool min3 = hasInterval (intervals, 3);
    const bool maj3 = hasInterval (intervals, 4);
    const bool dim5 = hasInterval (intervals, 6);
    const bool sus4 = hasInterval (intervals, 5) && ! min3 && ! maj3;
    const bool b7 = hasInterval (intervals, 10);
    const bool maj7 = hasInterval (intervals, 11);
    const bool sixth = hasInterval (intervals, 9);
    const bool ninth = hasInterval (intervals, 14);
    const bool eleventh = hasInterval (intervals, 17);
    const bool thirteenth = hasInterval (intervals, 21);
    const bool fifthOnly = intervals.size() <= 2 && hasInterval (intervals, 7);

    if (fifthOnly && ! min3 && ! maj3 && ! sus4)
        return "5";

    std::string core;
    if (sus4)
        core = "sus4";
    else if (min3 && dim5)
        core = "dim";
    else if (min3)
        core = "m";

    if (thirteenth)
    {
        if (maj7) return core + "maj13";
        if (b7 && min3) return "m13";
        if (b7) return "13";
        return core + "add13";
    }
    if (eleventh)
    {
        if (maj7) return core + "maj11";
        if (b7 && min3) return "m11";
        if (b7) return "11";
        return core + "add11";
    }
    if (ninth && sixth)
        return core.empty() ? "6/9" : core + "6/9";
    if (ninth)
    {
        if (sus4) return "9sus4";
        if (maj7) return core + "maj9";
        if (b7 && min3 && dim5) return "m9b5";
        if (b7 && min3) return "m9";
        if (b7) return "9";
        return core + "add9";
    }
    if (sixth && ! b7 && ! maj7)
        return core + "6";

    if (b7 || maj7)
    {
        if (sus4 && b7) return "7sus4";
        if (maj7) return core + "maj7";
        if (min3 && dim5) return "m7b5";
        if (min3) return "m7";
        return "7";
    }

    if (sus4)
        return "sus4";

    return core;
}

std::string decorateRoman (const std::string& base, const std::vector<int>& intervals)
{
    const bool sus4 = hasInterval (intervals, 5) && ! hasInterval (intervals, 3) && ! hasInterval (intervals, 4);
    const bool b7 = hasInterval (intervals, 10);
    const bool maj7 = hasInterval (intervals, 11);
    const bool ninth = hasInterval (intervals, 14);
    const bool eleventh = hasInterval (intervals, 17);
    const bool thirteenth = hasInterval (intervals, 21);
    const bool sixth = hasInterval (intervals, 9);
    const bool fifthOnly = intervals.size() <= 2 && hasInterval (intervals, 7) && ! hasInterval (intervals, 3)
                           && ! hasInterval (intervals, 4);

    std::string out = base;
    if (fifthOnly)
        return out + "5";
    if (sus4)
        out += "sus4";
    if (thirteenth) out += "13";
    else if (eleventh) out += "11";
    else if (ninth) out += "9";
    else if (maj7) out += "maj7";
    else if (b7) out += "7";
    else if (sixth) out += "6";
    return out;
}

void applyColor (std::vector<int>& intervals, ColorMode color, ExtensionStage stage)
{
    switch (color)
    {
        case ColorMode::Add6:
            if (stage >= ExtensionStage::Triad)
                addUnique (intervals, 9);
            break;
        case ColorMode::SixNine:
            if (stage >= ExtensionStage::Triad)
            {
                addUnique (intervals, 9);
                addUnique (intervals, 14);
            }
            break;
        case ColorMode::Quartal:
            intervals = { 0, 5, 10 };
            if (stage >= ExtensionStage::Seventh)
                addUnique (intervals, 15);
            if (stage >= ExtensionStage::Ninth)
                addUnique (intervals, 19);
            break;
        case ColorMode::Diatonic:
        default:
            break;
    }
}

} // namespace

int ChordEngine::layoutDegreePc (const ChordRequest& request) noexcept
{
    int keyPc = request.degreeKeyPc;
    if (keyPc >= 12)
        keyPc = 0;
    keyPc = wrapPc (keyPc);

    if (request.layout == LayoutMode::Static)
        return keyPc;

    return wrapPc (keyPc - request.tonalCenter);
}

ChordResult ChordEngine::resolve (const ChordRequest& request) const
{
    ChordResult result;
    result.shifted = request.shift;
    result.stage = stageFromExtensions (request.extensions);

    const auto scale = effectiveScale (request);
    const int tonic = wrapPc (request.tonalCenter);
    const int relPc = layoutDegreePc (request);
    const bool preferFlats = scalePrefersFlats (scale);
    const auto deg = scaleDegreeIndex (relPc, scale);

    std::vector<int> intervals;
    int rootRel = relPc;

    if (deg.has_value())
    {
        result.chromatic = false;
        const int idx = *deg;
        const int naturalThird = stackedInterval (scale, idx, 1);
        const bool minorDominantRaise = request.shift && naturalThird == 3 && idx == 4
                                        && stackedInterval (scale, idx, 2) == 7;

        intervals.push_back (0);

        if (result.stage == ExtensionStage::Fifths)
        {
            intervals.push_back (7);
        }
        else if (request.color == ColorMode::Quartal)
        {
            applyColor (intervals, request.color, result.stage);
        }
        else
        {
            int third = naturalThird;
            int fifth = stackedInterval (scale, idx, 2);

            if (minorDominantRaise)
            {
                third = 4;
                fifth = 7;
            }

            if (request.shift && result.stage <= ExtensionStage::Triad && ! minorDominantRaise)
            {
                intervals.push_back (5);
                intervals.push_back (fifth == 0 ? 7 : fifth);
            }
            else
            {
                intervals.push_back (third);
                intervals.push_back (fifth);
            }

            if (result.stage >= ExtensionStage::Seventh)
            {
                int seventh = stackedInterval (scale, idx, 3);
                if (request.shift || minorDominantRaise)
                    seventh = 10;
                intervals.push_back (seventh);
            }

            if (result.stage >= ExtensionStage::Ninth)
                intervals.push_back (stackedInterval (scale, idx, 4));
            if (result.stage >= ExtensionStage::Eleventh)
                intervals.push_back (stackedInterval (scale, idx, 5));
            if (result.stage >= ExtensionStage::Thirteenth)
                intervals.push_back (stackedInterval (scale, idx, 6));

            applyColor (intervals, request.color, result.stage);
        }

        result.roman = decorateRoman (diatonicRoman (scale, idx), intervals);
        result.function = kDegreeFunctions[static_cast<size_t> (idx)];
        if (minorDominantRaise)
        {
            result.roman = decorateRoman ("V", intervals);
            result.function = "dominant";
        }
        else if (request.shift && result.stage <= ExtensionStage::Triad)
        {
            result.function += " (sus)";
        }
    }
    else
    {
        result.chromatic = true;
        rootRel = wrapPc (relPc - 4);
        if (request.shift)
            rootRel = wrapPc (rootRel + 6);

        if (result.stage == ExtensionStage::Fifths)
        {
            intervals = { 0, 7 };
        }
        else if (request.color == ColorMode::Quartal)
        {
            applyColor (intervals, request.color, result.stage);
        }
        else
        {
            intervals = { 0, 4, 7 };
            if (result.stage >= ExtensionStage::Seventh)
                intervals.push_back (10);
            if (result.stage >= ExtensionStage::Ninth)
                intervals.push_back (14);
            if (result.stage >= ExtensionStage::Eleventh)
                intervals.push_back (17);
            if (result.stage >= ExtensionStage::Thirteenth)
                intervals.push_back (21);
            applyColor (intervals, request.color, result.stage);
        }

        const auto& table = preferFlats ? kMinorChromaticRoman : kMajorChromaticRoman;
        std::string base = table[static_cast<size_t> (relPc)];
        if (base.empty())
            base = "chrom";
        if (request.shift)
            base = "sub(" + base + ")";
        result.roman = decorateRoman (base, intervals);
        result.function = request.shift ? "tritone sub" : "secondary / interchange";
    }

    const int soundingRoot = wrapPc (rootRel + tonic);
    result.rootPc = soundingRoot;
    auto notes = voiceChord (intervals, soundingRoot, request.voicing);
    notes = applyInversion (std::move (notes), request.inversion);
    result.midiNotes = std::move (notes);
    result.bassNote = bassRegisterNote (request.inversion > 0 && ! result.midiNotes.empty()
                                            ? wrapPc (result.midiNotes.front())
                                            : soundingRoot,
                                        request.voicing);
    result.altBassNote = altBassRegisterNote (soundingRoot, 7, request.voicing);
    result.symbol = std::string (pcName (soundingRoot, preferFlats)) + qualitySuffix (intervals);

    std::ostringstream layout;
    layout << (request.layout == LayoutMode::Static ? "Static" : "Real") << " · "
           << pcName (tonic, preferFlats) << " "
           << kScaleNames[static_cast<size_t> (scale)];
    if (request.color != ColorMode::Diatonic)
        layout << " · " << kColorNames[static_cast<size_t> (request.color)];
    if (request.inversion > 0)
        layout << " · inv " << request.inversion;
    result.layoutLabel = layout.str();

    return result;
}

} // namespace anopi
