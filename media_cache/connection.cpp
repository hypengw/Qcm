#include "connection.h"

#include <asio/write.hpp>
#include <array>
#include <fstream>

#include <ctre.hpp>

#include "request/request.h"
#include "request/response.h"

#include "asio_helper/sync_file.h"

using namespace media_cache;

namespace
{
static constexpr auto             DigitPattern = ctll::fixed_string { "\\d*" };
static constexpr std::string_view DlSuffix { ".download" };

std::filesystem::path get_dl_path(std::filesystem::path p) { return p.replace_extension(DlSuffix); }
} // namespace

Connection::Connection(asio::ip::tcp::socket s, rc<DataBase> db): m_s(std::move(s)), m_db(db) {};
Connection::~Connection() {}

asio::awaitable<bool> Connection::check_cache(std::string_view      key,
                                              std::filesystem::path file_path) {
    auto has_entry = (bool)(co_await m_db->get(key));

    co_return has_entry&& std::filesystem::exists(file_path);
}

asio::awaitable<void> Connection::send_file_header(std::optional<DataBase::Item> db_item,
                                                   i64 offset, usize size) {
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

asio::awaitable<void> Connection::file_source(std::filesystem::path file_path) {
    helper::SyncFile file { std::fstream(file_path.native(), std::ios::in | std::ios::binary) };
    file.handle().exceptions(std::ios_base::badbit);
    auto size   = std::filesystem::file_size(file_path);
    auto offset = m_req->range_start.value_or(0);
    if (size <= (usize)offset) {
        ERROR_LOG("wrong range start {}", offset);
        co_return;
    }

    if (m_req->partial()) {
        file.handle().seekg(offset);
    }

    auto db_item = co_await m_db->get(m_req->proxy_id.value());
    co_await send_file_header(db_item, offset, size);

    std::vector<std::byte> buf;
    buf.resize(16 * 1024);

    for (;;) {
        file.read_some(asio::mutable_buffer((unsigned char*)buf.data(), buf.size()));
        auto size = file.handle().gcount();

        co_await asio::async_write(
            m_s, asio::buffer((unsigned char*)buf.data(), size), asio::use_awaitable);

        if (file.handle().eof()) break;
    }

    co_return;
}

asio::awaitable<void> Connection::send_http_header(DataBase::Item&         db_item,
                                                   const request::Header&  header,
                                                   const request::Request& proxy_req) {
    std::string rsp_header;

    rsp_header.append(header.contains("content-range") ? "HTTP/1.1 206 PARTIAL CONTENT\n"
                                                       : "HTTP/1.1 200 OK\n");
    if (header.contains("content-Length")) {
        auto len_str = header.at("content-length");
        rsp_header.append(fmt::format("Content-Length: {}\n", len_str));
        if (auto whole = ctre::starts_with<DigitPattern>(len_str); whole) {
            db_item.content_length = whole.to_number();
        }
    }
    if (header.contains("content-type")) {
        rsp_header.append(fmt::format("Content-Type: {}\n", header.at("content-type")));
        db_item.content_type = header.at("content-type");
    }
    if (header.contains("content-range")) {
        rsp_header.append(fmt::format("Content-Range: {}\n", header.at("content-range")));
        rsp_header.append("Accept-Ranges: bytes\n");
    }
    rsp_header.append("\r\n");

    DEBUG_LOG("rsp header, {}:\n{}", proxy_req.url(), rsp_header);

    co_await asio::async_write(
        m_s, asio::buffer(rsp_header.c_str(), rsp_header.size()), asio::use_awaitable);
}

asio::awaitable<void> Connection::http_source(std::filesystem::path file_path,
                                              rc<request::Session>  ses) {
    const auto& req      = m_req.value();
    std::string proxy_id = req.proxy_id.value();

    auto file_dl_path = get_dl_path(file_path);

    helper::SyncFile file { std::fstream(
        file_dl_path.native(),
        (std::filesystem::exists(file_dl_path) ? std::ios::in | std::ios::out : std::ios::out) |
            std::ios::binary) };
    file.handle().exceptions(std::ios_base::badbit);

    request::Request proxy_req;
    proxy_req.set_url(req.proxy_url.value()).set_transfer_timeout(120);

    if (req.range_start) {
        proxy_req.set_header("Range", fmt::format("bytes={}-", req.range_start.value()));
        file.handle().seekg(m_req->range_start.value());
    }

    auto rsp = (co_await ses->get(proxy_req)).value();

    DataBase::Item db_item;
    db_item.key = proxy_id;
    co_await send_http_header(db_item, rsp->header(), proxy_req);

    bool finished { false };

    auto check_finished = [&file_dl_path, &db_item, &file_path, &file, &rsp]() -> bool {
        if (auto cur_size = std::filesystem::file_size(file_dl_path);
            cur_size > 0 && cur_size == db_item.content_length) {
            file.handle().sync();
            std::error_code ec;
            std::filesystem::rename(file_dl_path, file_path, ec);
            if (ec) {
                ERROR_LOG("{}", ec.message());
            }
            DEBUG_LOG("finished: {}", file_path.native());
            rsp->cancel();
            return true;
        }
        return false;
    };

    std::vector<std::byte> buf;
    buf.resize(16 * 1024);
    auto buf_data = (unsigned char*)buf.data();

    for (;;) {
        if (! finished) {
            if (finished = check_finished(); finished) {
                co_await m_db->insert(db_item);
                continue;
            }

            auto [ec, size] = co_await asio::async_read(*rsp,
                                                        asio::mutable_buffer(buf_data, buf.size()),
                                                        asio::as_tuple(asio::use_awaitable));

            co_await asio::async_write(m_s, asio::buffer(buf_data, size), asio::use_awaitable);

            file.write_some(asio::buffer(buf_data, size));

            if (ec) {
                if (ec != asio::error::eof) {
                    ERROR_LOG("{}", ec.message());
                    m_s.cancel();
                }
                if (check_finished()) {
                    co_await m_db->insert(db_item);
                }
                break;
            }
        } else {
            file.read_some(asio::mutable_buffer(buf_data, buf.size()));
            auto size = file.handle().gcount();

            co_await asio::async_write(m_s, asio::buffer(buf_data, size), asio::use_awaitable);

            if (file.handle().eof()) break;
        }
    }
}

asio::awaitable<void> Connection::run(rc<request::Session> ses, std::filesystem::path cache_dir) {
    auto req = co_await GetRequest::read(m_s);
    m_req    = req;

    DEBUG_LOG("connection started: {}", m_req->proxy_id.value_or(""));

    if (req.proxy_id) {
        auto proxy_id = req.proxy_id.value();
        std::filesystem::create_directories(cache_dir);
        auto file = cache_dir / proxy_id;
        if (co_await check_cache(proxy_id, file)) {
            std::filesystem::remove(get_dl_path(file));
            co_await file_source(file);
        } else if (req.proxy_url) {
            co_await http_source(file, ses);
        }
    } else {
        ERROR_LOG("wrong path {}", req.path);
    }

    DEBUG_LOG("connection ended: {}", m_req->proxy_id.value_or(""));
    co_return;
}

void Connection::stop() { m_s.close(); }
