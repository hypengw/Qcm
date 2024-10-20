#pragma once

#include <filesystem>
#include <QtCore/QThread>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <asio/dispatch.hpp>

#include "core/log.h"
#include "core/qstr_helper.h"
#include "asio_qt/qt_executor.h"

namespace helper
{

class SqlConnect : public std::enable_shared_from_this<SqlConnect> {
public:
    SqlConnect(const std::filesystem::path& path, QStringView name)
        : m_db_path(path),
          m_name(name),
          m_thread(),
          m_ctx(make_rc<QtExecutionContext>(&m_thread, (QEvent::Type)QEvent::registerEventType())),
          m_ex(m_ctx) {
        m_thread.start();
        asio::dispatch(m_ex, [this]() {
            connect_db();
        });
    }
    ~SqlConnect() {
        m_thread.quit();
        m_thread.wait();
    }

    auto get_executor() -> QtExecutor& { return m_ex; }

    auto is_open() const -> bool { return m_db.isOpen(); }

    auto query() const -> QSqlQuery { return QSqlQuery(m_db); }
    auto db() -> QSqlDatabase& { return m_db; }
    auto error_str() -> QString { return m_db.lastError().text(); }

private:
    void connect_db() {
        m_db   = QSqlDatabase::addDatabase("QSQLITE", m_name);
        auto p = m_db_path;
        m_db.setDatabaseName(p.native().c_str());

        if (m_db.open()) {
        } else {
            ERROR_LOG("{}", m_db.lastError().text());
        }
    }

    std::filesystem::path  m_db_path;
    QString                m_name;
    QThread                m_thread;
    rc<QtExecutionContext> m_ctx;
    QtExecutor             m_ex;
    QSqlDatabase           m_db;
};

} // namespace helper