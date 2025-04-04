#pragma once

#include <variant>
#include <QObject>

import qcm.core;
#include "core/core.h"
#include "Qcm/util/async.hpp"

namespace qcm::state
{
class AppState : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QAsyncResult* rescue READ rescue CONSTANT FINAL)
public:
    AppState(QObject* parent = nullptr);
    ~AppState();

    struct Loading {
        bool operator==(const Loading&) const = default;
    };
    struct Start {
        bool operator==(const Start&) const = default;
    };
    struct Session {
        QObject* session;
        bool     operator==(const Session&) const = default;
    };
    struct Error {
        QString err;
        bool    fatal { false };
        bool    operator==(const Error&) const = default;
    };

    using StateTypelist = ycore::type_list<Loading, Start, Session, Error>;
    using State         = StateTypelist::to<std::variant>;

    template<typename T>
    auto is_state() const -> bool {
        return state().index() == StateTypelist::index<T>();
    }

    auto state() const -> const State&;
    void set_state(const State&);
    auto rescue() const -> QAsyncResult*;

    void load_session();

    Q_SIGNAL void stateChanged();

    Q_SIGNAL void start();
    Q_SIGNAL void session(QObject*);
    Q_SIGNAL void error(QString);

private:
    State         m_state;
    QAsyncResult* m_rescue;
};
} // namespace qcm::state