#include "connection.h"

#include <asio/write.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/as_tuple.hpp>
#include <asio/read.hpp>
#include <array>
#include <fstream>
#include <algorithm>
#include <cctype>

#include <ctre.hpp>

#include "core/strv_helper.h"

#include "asio_helper/sync_file.h"

using namespace media_cache;

namespace
{
static constexpr auto             DigitPattern = ctll::fixed_string { "\\d*" };
static constexpr std::string_view DlSuffix { ".download" };
static constexpr usize            WriteBuf { 4096 * 4 };

std::filesystem::path get_dl_path(std::filesystem::path p) { return p.replace_extension(DlSuffix); }

void init_db_item(const ncrequest::HttpHeader::Field& f, DataBase::Item& db_item) {
    if (helper::case_insensitive_compare(f.name, "content-length") == 0) {
        if (auto whole = ctre::starts_with<DigitPattern>(f.value); whole) {
            auto length = whole.to_number();
            if (length > 0 && db_item.content_length < (usize)length)
                db_item.content_length += length;
        }
    } else if (helper::case_insensitive_compare(f.name, "content-type") == 0) {
        db_item.content_type = f.value;
    } else if (helper::case_insensitive_compare(f.name, "content-range") == 0) {
        auto pos = f.value.find_last_of('/');
        if (pos != std::string_view::npos && f.value.size() >= pos) {
            auto num_str = std::string_view { f.value.begin() + pos + 1, f.value.end() };
            if (auto whole = ctre::starts_with<DigitPattern>(num_str); whole) {
                auto length = whole.to_number();
                if (length > 0 && db_item.content_length < (usize)length)
                    db_item.content_length = length;
            }
        }
    }
}
void init_db_item(const ncrequest::HttpHeader& header, DataBase::Item& db_item) {
    for (auto& f : header.fields) {
        init_db_item(f, db_item);
    }
}

} // namespace

Connection::Connection(asio::ip::tcp::socket s, rc<DataBase> db): m_s(std::move(s)), m_db(db) {};
Connection::~Connection() {}

auto Connection::get_req() -> const std::optional<GetRequest>& { return m_req; }

auto Connection::check_cache(std::string           key,
                             std::filesystem::path file_path) -> asio::awaitable<bool> {
    auto has_entry = (bool)(co_await m_db->get(key));

    co_return has_entry&& std::filesystem::exists(file_path);
}

auto Connection::send_file_header(std::optional<DataBase::Item> db_item, i64 offset,
                                  usize size) -> asio::awaitable<void> {
    std::string rsp_header;

    rsp_header.append(m_req->partial() ? "HTTP/1.1 206 PARTIAL CONTENT\n" : "HTTP/1.1 200 OK\n");
    rsp_header.append(fmt::format("Content-Length: {}\n", size - offset));
    if (db_item) {
        rsp_header.append(fmt::format("Content-Type: {}\n", db_item->content_type));
    }
    if (m_req->partial()) {
        rsp_header.append(fmt::format("Content-Range: bytes {}-{}/{}\n", offset, size - 1, size));
        rsp_header.append("Accept-Ranges: bytes\n");
    }

    rsp_header.append("\r\n");
    co_await asio::async_write(
        m_s, asio::buffer(rsp_header.c_str(), rsp_header.size()), asio::use_awaitable);
    co_return;
}

auto Connection::file_source(std::filesystem::path file_path, rc<Fallbacks> fbs,
                             std::pmr::polymorphic_allocator<byte> allocator)
    -> asio::awaitable<void> {
    helper::SyncFile file { std::fstream(file_path.native(), std::ios::in | std::ios::binary) };
    file.handle().exceptions(std::ios_base::badbit);
    auto size   = std::filesystem::file_size(file_path);
    auto offset = m_req->range_start.value_or(0);
    if (size <= (usize)offset) {
        ERROR_LOG("wrong range start {}", offset);
        co_return;
    }

    if (fbs->fragment) fbs->fragment(0, size, size);

    if (m_req->partial()) {
        file.handle().seekg(offset);
    }

    auto db_item = co_await m_db->get(m_req->proxy_id.value());
    co_await send_file_header(db_item, offset, size);

    std::pmr::vector<std::byte> buf(allocator);
    buf.resize(WriteBuf);

    for (;;) {
        file.read_some(asio::mutable_buffer((unsigned char*)buf.data(), buf.size()));
        auto size = file.handle().gcount();

        co_await asio::async_write(
            m_s, asio::buffer((unsigned char*)buf.data(), size), asio::use_awaitable);

        if (file.handle().eof()) break;
    }

    co_return;
}

auto Connection::send_http_header(DataBase::Item& db_item, const ncrequest::HttpHeader& header,
                                  const ncrequest::Request& proxy_req) -> asio::awaitable<void> {
    std::string rsp_header;

    bool has_range = header.has_field("content-range");
    rsp_header.append(has_range ? "HTTP/1.1 206 PARTIAL CONTENT\n" : "HTTP/1.1 200 OK\n");
    for (auto& f : header.fields) {
        init_db_item(f, db_item);
        if (helper::case_insensitive_compare(f.name, "content-range") == 0) {
            rsp_header.append("Accept-Ranges: bytes\n");
        } else if (helper::case_insensitive_compare(f.name, "host") == 0) {
            continue;
        }
        rsp_header.append(fmt::format("{}: {}\n", f.name, f.value));
    }

    rsp_header.append("\r\n");

    DEBUG_LOG("rsp header, {}:\n{}", proxy_req.url(), rsp_header);

    co_await asio::async_write(
        m_s, asio::buffer(rsp_header.c_str(), rsp_header.size()), asio::use_awaitable);
}

auto Connection::http_source(std::filesystem::path file_path, rc<ncrequest::Session> ses,
                             rc<Writer> writer) -> asio::awaitable<void> {
    const auto& req = m_req.value();

    struct Ctx {
        std::filesystem::path              file_dl_path;
        rc<Writer::File>                   file;
        DataBase::Item                     db_item;
        std::shared_ptr<ncrequest::Response> rsp;
    };
    Ctx ctx;

    ctx.file_dl_path = get_dl_path(file_path);
    ctx.file         = writer->create(ctx.file_dl_path);

    bool need_pre_download { false };
    {
        std::string      proxy_id = req.proxy_id.value();
        ncrequest::Request proxy_req;
        {
            proxy_req.set_url(req.proxy_url.value());
            for (auto& f : req.header.fields) {
                if (helper::case_insensitive_compare(f.name, "host") == 0) continue;
                proxy_req.set_header(f.name, f.value);
            }
            proxy_req.get_opt<ncrequest::req_opt::Timeout>().set_transfer_timeout(180);
            proxy_req.get_opt<ncrequest::req_opt::Tcp>().set_keepalive(true);
        }

        if (req.range_start) {
            ctx.file->handle().seekg(m_req->range_start.value());
        }

        // do request
        ctx.rsp = (co_await ses->get(proxy_req)).value();

        ctx.db_item.key   = proxy_id;
        auto code         = ctx.rsp->code().value_or(0);
        need_pre_download = code != 206 && req.range_start;
        if (! need_pre_download) {
            ctx.db_item.content_length = req.range_start.value();
            co_await send_http_header(ctx.db_item, ctx.rsp->header(), proxy_req);
            ctx.file->set_expected_size(ctx.db_item.content_length);
            co_await m_db->insert(ctx.db_item);
        } else {
            // full download
            ctx.rsp->cancel();
            proxy_req.remove_header("Range");
            proxy_req.remove_header("range");

            // do full request
            ctx.rsp = (co_await ses->get(proxy_req)).value();

            ctx.file->handle().seekg(0);
            if (auto db_opt = co_await m_db->get(proxy_id)) {
                ctx.db_item = db_opt.value();
            } else {
                init_db_item(ctx.rsp->header(), ctx.db_item);
            }
            assert(ctx.db_item.content_length > 0);
            ctx.file->set_expected_size(ctx.db_item.content_length);
            co_await m_db->insert(ctx.db_item);
        }
    }

    auto check_finished = [&ctx, file_path]() -> bool {
        if (ctx.file->is_fill(ctx.db_item.content_length)) {
            ctx.file->handle().sync();
            std::error_code ec;
            std::filesystem::rename(ctx.file_dl_path, file_path, ec);
            if (ec) {
                ERROR_LOG("{}", ec.message());
            }
            DEBUG_LOG("finished: {}", file_path.native());
            ctx.rsp->cancel();
            return true;
        }
        return false;
    };

    std::pmr::vector<std::byte> buf(ses->allocator());
    buf.resize(WriteBuf);
    auto buf_data = (unsigned char*)buf.data();

    bool finished { false };
    for (;;) {
        if (! finished) {
            if (finished = check_finished(); finished) {
                if (need_pre_download) {
                    co_await send_file_header(
                        ctx.db_item, req.range_start.value_or(0), ctx.db_item.content_length);
                }
                continue;
            }

            auto [ec, size] = co_await asio::async_read(*ctx.rsp,
                                                        asio::mutable_buffer(buf_data, buf.size()),
                                                        asio::as_tuple(asio::use_awaitable));

            if (! need_pre_download) {
                co_await asio::async_write(m_s, asio::buffer(buf_data, size), asio::use_awaitable);
            }

            co_await asio::async_write(
                *ctx.file, asio::buffer(buf_data, size), asio::use_awaitable);

            if (ec) {
                if (ec != asio::error::eof) {
                    ERROR_LOG("{}", ec.message());
                    m_s.cancel();
                }
                if (finished = check_finished(); finished) {
                    if (need_pre_download) {
                        co_await send_file_header(
                            ctx.db_item, req.range_start.value_or(0), ctx.db_item.content_length);
                        continue;
                    }
                }
                break;
            }
        } else {
            DEBUG_LOG("using cache file");
            ctx.file->read_some(asio::mutable_buffer(buf_data, buf.size()));
            auto size = ctx.file->handle().gcount();

            co_await asio::async_write(m_s, asio::buffer(buf_data, size), asio::use_awaitable);

            if (ctx.file->handle().eof()) break;
        }
    }
}

auto get_proxyid_dir(std::string_view proxy_id) -> std::string {
    auto out = proxy_id.size() >= 2 ? proxy_id.substr(0, 2) : "no"sv;
    return helper::to_upper(out);
}

auto Connection::run(rc<ncrequest::Session> ses, rc<Writer> writer, rc<Fallbacks> fbs,
                     std::filesystem::path cache_dir) -> asio::awaitable<void> {
    auto req = co_await GetRequest::read(m_s);
    m_req    = req;

    DEBUG_LOG("connection started: {}", m_req->proxy_id.value_or(""));

    if (req.proxy_id) {
        auto proxy_id = req.proxy_id.value();
        cache_dir     = cache_dir / get_proxyid_dir(proxy_id);
        std::filesystem::create_directories(cache_dir);
        auto file = cache_dir / proxy_id;
        if (co_await check_cache(proxy_id, file)) {
            std::filesystem::remove(get_dl_path(file));
            co_await file_source(file, fbs, ses->allocator());
        } else if (req.proxy_url) {
            co_await http_source(file, ses, writer);
        }
    } else {
        ERROR_LOG("wrong path {}", req.path);
    }

    DEBUG_LOG("connection ended: {}", m_req->proxy_id.value_or(""));
    co_return;
}

void Connection::stop() { m_s.close(); }
