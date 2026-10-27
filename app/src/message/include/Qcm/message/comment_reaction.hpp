#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>

namespace qcm::comments
{
class Reaction {
public:
    std::optional<bool>          liked;
    std::optional<std::uint64_t> count;
    bool                         pending {};
    bool                         uncertain {};

    void read(std::optional<bool> value, std::optional<std::uint64_t> total,
              std::uint64_t revision) {
        if (pending || revision < m_barrier) return;
        liked     = value;
        count     = total;
        uncertain = false;
    }
    bool begin(std::uint64_t revision) {
        if (pending || uncertain || ! liked.has_value()) return false;
        pending   = true;
        m_barrier = revision;
        return true;
    }
    void complete(bool value, std::optional<std::uint64_t> total, std::uint64_t revision) {
        if (! pending) return;
        if (total)
            count = total;
        else if (liked && *liked != value && count) {
            if (value && *count < std::numeric_limits<std::uint64_t>::max())
                ++*count;
            else if (! value && *count)
                --*count;
        }
        liked     = value;
        pending   = false;
        uncertain = false;
        m_barrier = revision;
    }
    void fail(std::uint64_t revision) {
        pending   = false;
        uncertain = true;
        m_barrier = revision;
    }

private:
    std::uint64_t m_barrier {};
};
} // namespace qcm::comments
