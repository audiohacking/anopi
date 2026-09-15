#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

namespace anopi
{

enum class ControlAction : uint8_t
{
    Degree0 = 0,
    Degree12 = 12,
    Tonal0 = 20,
    Tonal11 = 31,
    ExtUp = 40,
    ExtDown,
    VoicingUp,
    VoicingDown,
    BendUp,
    BendDown,
    ToggleTonality,
    ToggleLayout,
    ShiftHold,
    Sustain,
    BassRoot,
    BassAlt,
    Strum0,
    Strum1,
    Strum2,
    Strum3,
    MuteKeys,
    MuteBass,
    MuteArp,
    MutePad,
    Unknown
};

struct MidiBinding
{
    int cc = -1;
    int note = -1;
    int channel = 0; // 0 = omni
};

class ControlMap
{
public:
    ControlMap();

    static std::optional<int> qwertyDegree (int keyCode) noexcept;
    static std::optional<int> qwertyTonal (int keyCode) noexcept;
    static std::optional<int> qwertyStrum (int keyCode) noexcept;
    static bool isBassRootKey (int keyCode) noexcept;
    static bool isBassAltKey (int keyCode) noexcept;
    static int degreeKeyCode (int degree) noexcept;
    static int tonalKeyCode (int pc) noexcept;

    static constexpr std::array<int, 13> kDegreeKeyCodes {
        'A', 'W', 'S', 'E', 'D', 'F', 'T', 'G', 'Y', 'H', 'U', 'J', 'K'
    };
    static constexpr std::array<int, 12> kTonalKeyCodes {
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '='
    };
    static constexpr std::array<int, 4> kStrumKeyCodes { 'Z', 'X', 'C', 'V' };

    MidiBinding extensions { 16, -1, 0 };
    MidiBinding voicing { 17, -1, 0 };
    MidiBinding tonality { 18, -1, 0 };
    MidiBinding layout { 19, -1, 0 };
    MidiBinding muteKeys { 20, -1, 0 };
    MidiBinding muteBass { 21, -1, 0 };
    MidiBinding muteArp { 22, -1, 0 };
    MidiBinding mutePad { 23, -1, 0 };
    MidiBinding sustain { 64, -1, 0 };

    std::atomic<int> learnTarget { -1 }; // 0 ext, 1 voicing, ...
    std::atomic<int> lastFired { -1 };

    void armLearn (int target) noexcept { learnTarget.store (target); }
    bool consumeLearnCC (int cc, int channel) noexcept;
};

} // namespace anopi
