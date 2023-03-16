#pragma once
#include <QObject>
#include <QQmlApplicationEngine>

// #define ASIO_ENABLE_HANDLER_TRACKING

#include <iostream>
#include <vector>
#include "qt_executor.h"
#include "request/request.h"
#include "request/session.h"
#include "request/response.h"
#include <fmt/core.h>

class Test {
public:
    void ok() { std::cout << "ok" << std::endl; }
    ~Test() { std::cout << "discourd" << std::endl; }
};

class CppInterface : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    CppInterface(QObject* parent = nullptr)
        : QObject(parent),
          m_executor(std::make_shared<QtExecutionContext>(this)),
          m_session(std::make_shared<request::Session>(m_executor)) {}
    virtual ~CppInterface() {}

    Q_INVOKABLE QString test() {
        asio::co_spawn(
            m_executor,
            [this]() -> asio::awaitable<void> {
                auto req = request::Request("http://www.baidu.com");
                req.set_connect_timeout(4);
                request::UrlParams param;
                param.set_param("nihao", "ok");
                param.set_param("login", "nnn");
                auto res_opt = co_await m_session->post(req, asio::buffer(param.encode()));
                if (! res_opt.has_value()) co_return;
                auto res = res_opt.value();

                asio::streambuf buf;
                fmt::print("start read");
                auto [ec, size] = co_await asio::async_read(
                    *res, buf, asio::transfer_all(), asio::as_tuple(asio::use_awaitable));
                if (auto opt = res->attribute<request::Attribute::HttpCode>(); opt.has_value()) {
                    fmt::print("code: {}\n", opt.value());
                }
                auto data = buf.data();
                fmt::print("{}\n end\n", std::string_view((const char*)data.data(), data.size()));
                std::fflush(nullptr);
            },
            asio::detached);
        return u"ok"_qs;
    }

private:
    rc<asio::steady_timer> m_timer;
    QtExecutor             m_executor;
    rc<request::Session>   m_session;
};
