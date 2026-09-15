#include "ControlMap.h"

#include <algorithm>

namespace anopi
{

ControlMap::ControlMap() = default;

std::optional<int> ControlMap::qwertyDegree (int keyCode) noexcept
{
    for (int i = 0; i < 13; ++i)
        if (kDegreeKeyCodes[static_cast<size_t> (i)] == keyCode)
            return i;
    return std::nullopt;
}

std::optional<int> ControlMap::qwertyTonal (int keyCode) noexcept
{
    for (int i = 0; i < 12; ++i)
        if (kTonalKeyCodes[static_cast<size_t> (i)] == keyCode)
            return i;
    return std::nullopt;
}

std::optional<int> ControlMap::qwertyStrum (int keyCode) noexcept
{
    for (int i = 0; i < 4; ++i)
        if (kStrumKeyCodes[static_cast<size_t> (i)] == keyCode)
            return i;
    return std::nullopt;
}

bool ControlMap::isBassRootKey (int keyCode) noexcept { return keyCode == 'B'; }
bool ControlMap::isBassAltKey (int keyCode) noexcept { return keyCode == 'N'; }

int ControlMap::degreeKeyCode (int degree) noexcept
{
    degree = std::clamp (degree, 0, 12);
    return kDegreeKeyCodes[static_cast<size_t> (degree)];
}

int ControlMap::tonalKeyCode (int pc) noexcept
{
    pc = std::clamp (pc, 0, 11);
    return kTonalKeyCodes[static_cast<size_t> (pc)];
}

bool ControlMap::consumeLearnCC (int cc, int /*channel*/) noexcept
{
    const int target = learnTarget.exchange (-1);
    if (target < 0)
        return false;

    MidiBinding* b = nullptr;
    switch (target)
    {
        case 0: b = &extensions; break;
        case 1: b = &voicing; break;
        case 2: b = &tonality; break;
        case 3: b = &layout; break;
        case 4: b = &muteKeys; break;
        case 5: b = &muteBass; break;
        case 6: b = &muteArp; break;
        case 7: b = &mutePad; break;
        case 8: b = &sustain; break;
        default: break;
    }

    if (b == nullptr)
        return false;

    b->cc = cc;
    return true;
}

} // namespace anopi
