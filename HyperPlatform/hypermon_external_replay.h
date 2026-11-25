#pragma once

namespace hypermon::interrupt_replay {

inline constexpr unsigned int kEntryValid{1U << 31};
inline constexpr unsigned int kEntryTypeAndErrorMask{0xfU << 8};
inline constexpr unsigned int kFirstExternalVector{16};
inline constexpr unsigned int kVectorCount{256};
inline constexpr unsigned int kInterruptWindow{1U << 2};

// A physical vector can have one acknowledged delivery outstanding on this
// processor. A fresh hardware request for that vector remains in the APIC.
struct State {
    unsigned long long pending[4];
    bool owns_window;
};

constexpr bool IsExternal(const unsigned int info)
{
    return (info & (kEntryValid | kEntryTypeAndErrorMask)) == kEntryValid &&
           (info & 0xffU) >= kFirstExternalVector;
}

constexpr bool HasPending(const State& state)
{
    return (state.pending[0] | state.pending[1] | state.pending[2] | state.pending[3]) != 0;
}

constexpr void Observe(State& state, const unsigned int vectoring)
{
    if (IsExternal(vectoring)) {
        const auto vector = vectoring & 0xffU;
        state.pending[vector / 64] |= 1ULL << (vector % 64);
    }
}

constexpr void Retire(State& state, const unsigned int vector)
{
    state.pending[vector / 64] &= ~(1ULL << (vector % 64));
}

template <typename WriteEntry>
bool PrepareEntry(State& state, const unsigned int entry, const bool guest_ready,
                  const WriteEntry& write_entry)
{
    if ((entry & kEntryValid) != 0) {
        if (IsExternal(entry)) {
            Retire(state, entry & 0xffU);
        }
        return true;
    }
    if (!guest_ready) {
        return true;
    }
    for (auto vector = kVectorCount; vector > kFirstExternalVector;) {
        --vector;
        if ((state.pending[vector / 64] & (1ULL << (vector % 64))) == 0) {
            continue;
        }
        if (!write_entry(kEntryValid | vector)) {
            return false;
        }
        Retire(state, vector);
        break;
    }
    return true;
}

template <typename WritePrimary>
bool UpdateWindow(State& state, const unsigned int primary, const WritePrimary& write_primary)
{
    const bool enabled = (primary & kInterruptWindow) != 0;
    if (HasPending(state)) {
        if (!enabled) {
            if (!write_primary(primary | kInterruptWindow)) {
                return false;
            }
            state.owns_window = true;
        }
    } else if (state.owns_window) {
        if (enabled && !write_primary(primary & ~kInterruptWindow)) {
            return false;
        }
        state.owns_window = false;
    }
    return true;
}

} // namespace hypermon::interrupt_replay
