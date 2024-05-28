#pragma once
#include <map>
#include <atomic>
#include "core/core.h"

namespace media_cache
{
class Fragment {
public:
    auto is_fill(usize size) const -> bool { return m_size == size; }
    void write_size(usize pos, usize size) { write(pos, pos + size); }
    void write(usize start, usize end) {
        auto insert_it = m_frags.lower_bound(start);
        if (insert_it != m_frags.begin()) {
            insert_it--;
            if (insert_it->second < start) {
                m_frags.insert({ start, end });
                insert_it++;
            }
        } else if (insert_it == m_frags.end()) {
            m_frags.insert({ start, end });
            return;
        }

        auto it = insert_it;
        it++;
        while (it != m_frags.end() && it->first < end) {
            auto cur = it;
            it++;
            if (end < cur->second) {
                end = cur->second;
            }
            m_frags.erase(cur);
        }
        insert_it->second = std::max(insert_it->second, end);

        if (m_frags.size() == 1) {
            m_size = m_frags.begin()->second - m_frags.begin()->first;
        }
    }

    void merge(const Fragment& o) {
        for (auto& el : o.m_frags) {
            write(el.first, el.second);
        }
    }

private:
    std::atomic<usize> m_size;
    // start, end
    std::map<usize, usize> m_frags;
};
} // namespace media_cache