#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace anopi
{

struct LiveEvent
{
    enum class Type : uint8_t
    {
        DegreeOn,
        DegreeOff,
        StrumOn,
        StrumOff,
        BassRootOn,
        BassRootOff,
        BassAltOn,
        BassAltOff,
        PitchBend,
        AllNotesOff
    };

    Type type {};
    uint8_t a = 0;
    uint8_t b = 0;
    int extra = 0;
};

class LiveEventFifo
{
public:
    bool push (const LiveEvent& event) noexcept
    {
        const int w = write.load (std::memory_order_relaxed);
        const int n = (w + 1) & kMask;
        if (n == read.load (std::memory_order_acquire))
            return false;
        buffer[static_cast<size_t> (w)] = event;
        write.store (n, std::memory_order_release);
        return true;
    }

    bool pop (LiveEvent& event) noexcept
    {
        const int r = read.load (std::memory_order_relaxed);
        if (r == write.load (std::memory_order_acquire))
            return false;
        event = buffer[static_cast<size_t> (r)];
        read.store ((r + 1) & kMask, std::memory_order_release);
        return true;
    }

private:
    static constexpr int kSize = 256;
    static constexpr int kMask = kSize - 1;
    std::array<LiveEvent, kSize> buffer {};
    std::atomic<int> write { 0 };
    std::atomic<int> read { 0 };
};

} // namespace anopi
