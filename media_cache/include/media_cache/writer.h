#pragma once

#include <filesystem>
#include <mutex>
#include <thread>
#include <deque>

#include <asio/post.hpp>
#include <asio/thread_pool.hpp>

#include "media_cache/fragment.h"
#include "media_cache/fallbacks.h"
#include "asio_helper/sync_file.h"

namespace media_cache
{

class Writer {
public:
    using executor_type = asio::thread_pool::executor_type;
    static constexpr usize RecordNum { 10 };

    class File : std::enable_shared_from_this<File> {
        friend class Writer;

    public:
        using executor_type = Writer::executor_type;
        File(executor_type ex, std::fstream&& f, rc<Fragment> frag)
            : m_ex(ex), m_file(std::move(f)), m_frag(frag) {}
        ~File() {}

        auto is_fill(usize s) const -> bool { return m_frag->is_fill(s); }
        void set_expected_size(usize val) { m_frag->set_expected_size(val); };

        auto& handle() { return m_file.handle(); }

        template<typename MB>
            requires asio::is_mutable_buffer_sequence<MB>::value
        auto read_some(MB buffer) {
            return m_file.read_some(buffer);
        }

        template<typename MB, typename CompletionToken>
            requires asio::is_const_buffer_sequence<MB>::value
        auto async_write_some(const MB& buffer, CompletionToken&& token) {
            using ret = void(asio::error_code, usize);
            return asio::async_initiate<CompletionToken, ret>(
                [&](auto&& handler) mutable {
                    asio::post(m_ex,
                               [this, buffer = buffer, handler = std::move(handler)]() mutable {
                                   auto start_i64 = m_file.handle().tellg();
                                   if (m_file.handle().fail()) {
                                       handler(std::error_code { errno, std::system_category() },
                                               (usize)0);
                                   }
                                   auto start  = usize(start_i64);
                                   auto size_i = m_file.write_some(buffer);
                                   if (m_file.handle().fail()) {
                                       handler(std::error_code { errno, std::system_category() },
                                               (usize)0);
                                   }
                                   auto size = (usize)size_i;

                                   m_frag->write(start, start + size);
                                   handler(asio::error_code {}, size);
                               });
                },
                token);
        }

    private:
        executor_type                  m_ex;
        helper::SyncFile<std::fstream> m_file;
        rc<Fragment>                   m_frag;
    };

    Writer(rc<Fallbacks> fbs): m_context(1), m_ex(m_context.get_executor()), m_fbs(fbs) {}
    ~Writer() {}

    auto create(std::filesystem::path path) -> rc<File> {
        rc<Fragment> frag;
        {
            std::unique_lock lock { m_mutex };
            if (! m_frags.contains(path)) {
                m_frags.insert({ path, make_rc<Fragment>(m_fbs) });
            }
            frag = m_frags.at(path);
            refresh_lru(path);
        }
        auto file =
            make_rc<File>(m_ex,
                          std::fstream(path.native(),
                                       (std::filesystem::exists(path) ? std::ios::in | std::ios::out
                                                                      : std::ios::out) |
                                           std::ios::binary),
                          frag);
        file->m_file.handle().exceptions(std::ios_base::badbit);
        return file;
    }

private:
    void refresh_lru(const std::filesystem::path& p) {
        auto it = std::find_if(m_lru.begin(), m_lru.end(), [&p](auto& p2) {
            return p == p2;
        });
        if (it != m_lru.end()) {
            *it = std::filesystem::path {};
        }
        m_lru.push_back(p);

        if (m_lru.size() > RecordNum) {
            if (! m_lru.front().empty()) {
                m_frags.erase(m_lru.front());
            }
            m_lru.pop_front();
        }
    }

private:
    asio::thread_pool m_context;
    executor_type     m_ex;
    rc<Fallbacks>     m_fbs;

    std::map<std::filesystem::path, rc<Fragment>> m_frags;
    std::deque<std::filesystem::path>             m_lru;
    std::mutex                                    m_mutex;
    std::thread                                   m_thread;
};

} // namespace media_cache