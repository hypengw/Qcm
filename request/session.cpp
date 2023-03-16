#include "request.h"
#include "request_p.h"

#include <asio/deferred.hpp>
#include <asio/co_spawn.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <iostream>

#include "session.h"
#include "session_p.h"

#include "response.h"
#include "curl_multi.h"
#include "curl_easy.h"

using namespace request;

Session::Session(asio::any_io_executor ex): m_p(std::make_unique<Private>(ex)) {}

Session::~Session() {}

asio::any_io_executor& Session::get_executor() {
    C_D(Session);
    return d->ex;
}

asio::awaitable<bool> Session::perform(const rc<Response>& rsp) {
    C_D(Session);
    rsp->prepare_perform();

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    d->rsps.insert(rsp);
    auto [mc] = co_await d->multi_info->async_perform(
        rsp->easy(), asio::bind_executor(get_executor(), asio::as_tuple(asio::use_awaitable)));
    if (mc != CURLM_OK) d->rsps.erase(rsp);

    co_return mc == CURLM_OK;
}

asio::awaitable<std::optional<rc<Response>>> Session::get(const Request& req) {
    C_D(Session);
    auto res = Response::make_response(req, Operation::GetOperation, shared_from_this());

    if (co_await perform(res)) co_return res;
    co_return std::nullopt;
}

asio::awaitable<std::optional<rc<Response>>> Session::post(const Request&     req,
                                                           asio::const_buffer buf) {
    C_D(Session);
    rc<Response> res = Response::make_response(req, Operation::PostOperation, shared_from_this());
    res->add_send_buffer(buf);

    d->rsps.insert(res);
    if (co_await perform(res)) co_return res;
    co_return std::nullopt;
}

Session::Private::Private(asio::any_io_executor ex) noexcept
    : multi_info(std::make_shared<CurlMulti>(ex)), ex(multi_info->get_strand()) {};

void Session::load_cookie(std::filesystem::path p) {
    C_D(Session);
    d->multi_info->load_cookie(p);
}
void Session::save_cookie(std::filesystem::path p) const {
    C_D(const Session);
    d->multi_info->save_cookie(p);
}

void Session::done(const rc<Response>& rsp) {
    C_D(Session);
    auto self = shared_from_this();
    asio::dispatch(d->ex, [self, d, rsp = rsp]() {
        if (d->rsps.contains(rsp)) {
            d->multi_info->remove_handle(rsp->easy());
            d->rsps.erase(rsp);
        }
    });
}

void Session::test() { C_D(Session); }
