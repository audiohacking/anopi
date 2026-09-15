#pragma once

#include <algorithm>
#include <vector>

namespace anopi
{

/** Fold pitch-classes into a compact register around a voicing slider (0–1). */
std::vector<int> voiceChord (const std::vector<int>& relativeIntervals,
                             int rootPc,
                             float voicing,
                             int bassOctaveMidi = 36);

int bassRegisterNote (int rootPc, float voicing) noexcept;
int altBassRegisterNote (int rootPc, int fifthOrThirdInterval, float voicing) noexcept;
std::vector<int> applyInversion (std::vector<int> notes, int inversion);

} // namespace anopi
