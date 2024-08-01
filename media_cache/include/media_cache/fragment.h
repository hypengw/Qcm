#pragma once
#include <map>
#include <atomic>
#include <limits>

#include "core/core.h"
#include "media_cache/fallbacks.h"
#include "core/log.h"

namespace media_cache
{
class Fragment {
public:
    Fragment(rc<Fallbacks> fbs): m_expected_size(0), m_fbs(fbs) {}
    ~Fragment() {}

    void set_expected_size(usize val) { m_expected_size = val; }

    auto is_fill(usize size) const -> bool { return m_size == size; }
    void write_size(usize pos, usize size) { write(pos, pos + size); }
    void write(usize start, usize end) {
        auto insert_it = m_frags.lower_bound(start);
        do {
            if (insert_it != m_frags.begin()) {
                insert_it--;
                if (insert_it->second < start) {
                    insert_it = m_frags.insert({ start, end }).first;
                }
            } else if (insert_it == m_frags.end()) {
                insert_it = m_frags.insert({ start, end }).first;
                break;
            }

            auto it = insert_it;
            it++;
            while (it != m_frags.end() && it->first <= end) {
                auto cur = it;
                it++;
                if (end < cur->second) {
                    end = cur->second;
                }
                m_frags.erase(cur);
            }
            insert_it->second = std::max(insert_it->second, end);
        } while (false);

        // callback
        if (m_fbs->fragment) {
            m_fbs->fragment(insert_it->first, insert_it->second, m_expected_size);
        }

        if (m_frags.size() == 1 && m_frags.begin()->first == 0) {
            m_size = m_frags.begin()->second;
        }
    }

    void merge(const Fragment& o) {
        for (auto& el : o.m_frags) {
            write(el.first, el.second);
        }
    }

private:
    std::atomic<usize> m_size;
    usize              m_expected_size;
    // start, end
    std::map<usize, usize> m_frags;
    rc<Fallbacks>          m_fbs;
};
} // namespace media_cache