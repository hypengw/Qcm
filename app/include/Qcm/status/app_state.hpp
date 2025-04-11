#pragma once

#include "core/core.h"

#include <variant>
#include <QQmlEngine>
#include "Qcm/util/async.hpp"

namespace qcm
{
class AppState : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
public:
    AppState(QObject* parent = nullptr);
    ~AppState();

    struct Loading {
        bool operator==(const Loading&) const = default;
    };
    struct Welcome {
        bool operator==(const Welcome&) const = default;
    };
    struct Main {
        bool operator==(const Main&) const = default;
    };
    struct Error {
        QString err;
        bool    fatal { false };
        bool    operator==(const Error&) const { return true; }
    };

    using StateTypelist = ycore::type_list<Loading, Welcome, Main, Error>;
    using State         = StateTypelist::to<std::variant>;

    template<typename T>
    auto is_state() const -> bool {
        return state().index() == StateTypelist::index<T>();
    }

    auto state() const -> const State&;
    void set_state(const State&);

    void load_session();

    Q_SIGNAL void stateChanged();
    Q_SIGNAL void retry();

    Q_SIGNAL void loading();
    Q_SIGNAL void welcome();
    Q_SIGNAL void main();
    Q_SIGNAL void error(QString);

    Q_SLOT void onQmlCompleted();

private:
    Q_SLOT void on_retry();
    void        triggerSignal();

    State m_state;
};
} // namespace qcm