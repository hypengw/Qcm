#pragma once
#include <curl/curl.h>
#include <list>
#include <utility>
#include <string>
#include <string_view>

namespace request
{

class CurlSlist {
public:
    struct Item {
        curl_slist  ptr {};
        std::string str;

        Item() noexcept {
            ptr.data = const_cast<char*>(str.c_str());
            ptr.next = NULL;
        }
        Item(std::string_view s) noexcept: str(s) {
            ptr.data = const_cast<char*>(str.c_str());
            ptr.next = NULL;
        }
        Item& operator=(std::string_view s) {
            str      = s;
            ptr.data = const_cast<char*>(str.c_str());
            ptr.next = NULL;
            return *this;
        }

        void update_next(Item& next) {
            ptr.data = const_cast<char*>(str.c_str());
            ptr.next = &next.ptr;
        }
        void set_null() { ptr.next = NULL; }
    };
    using base           = std::list<Item>;
    using value_type     = base::value_type;
    using size_type      = base::size_type;
    using iterator       = base::iterator;
    using const_iterator = base::const_iterator;
    // iterator
    auto begin() noexcept { return m_list.begin(); }
    auto begin() const noexcept { return m_list.begin(); }
    auto cbegin() const noexcept { return m_list.cbegin(); }
    auto end() noexcept { return m_list.end(); }
    auto end() const noexcept { return m_list.end(); }
    auto cend() const noexcept { return m_list.cend(); }

    // ele access
    auto front() noexcept { return m_list.front(); }
    auto front() const noexcept { return m_list.front(); }
    auto back() noexcept { return m_list.back(); }
    auto back() const noexcept { return m_list.back(); }

    // capacity
    auto empty() const noexcept { return m_list.empty(); }
    auto size() const noexcept { return m_list.size(); }
    auto max_size() const noexcept { return m_list.max_size(); }
    void resize(size_type s) { m_list.resize(s); }
    void resize(size_type s, const value_type& v) { m_list.resize(s, v); }

    // constructor & operator
    CurlSlist() noexcept = default;
    CurlSlist(const CurlSlist& o) noexcept: m_list(o.m_list) { update_ptr(); }
    CurlSlist& operator=(const CurlSlist& o) {
        m_list = o.m_list;
        update_ptr();
        return *this;
    }
    CurlSlist(CurlSlist&& o) noexcept: m_list(std::exchange(o.m_list, {})) {}
    CurlSlist& operator=(CurlSlist&& o) noexcept {
        m_list = std::exchange(o.m_list, {});
        return *this;
    }
    template<class InputIt>
    CurlSlist(InputIt first, InputIt last) {
        for (; first != last; first++) {
            push_back(std::string_view(*first));
        }
    }

    void update_ptr() {
        for (auto it = begin(); it != end(); it++) {
            update_next(it);
        }
    }
    // Modify
    void clear() noexcept { m_list.clear(); }
    void push_front(value_type&& value) {
        m_list.push_front(std::forward<value_type>(value));
        update_next(begin());
    }
    void pop_front() { m_list.pop_front(); }
    void push_back(value_type&& value) {
        m_list.push_back(std::forward<value_type>(value));
        update_next(--end());
    }
    void pop_back() {
        m_list.pop_back();
        if (! empty()) {
            back().set_null();
        }
    }
    auto erase(const_iterator pos) {
        auto next = m_list.erase(pos);
        if (next != begin()) {
            auto pre = next;
            pre--;
            update_next(pre);
        }
        return next;
    }

    curl_slist* handle() {
        if (empty())
            return NULL;
        else
            return &(begin()->ptr);
    }

private:
    void update_next(iterator it) {
        if (it != end()) {
            auto next = it;
            next++;
            if (next != end()) {
                it->update_next(*next);
            } else {
                it->set_null();
            }
        }
    }

    base m_list;
};
} // namespace request
