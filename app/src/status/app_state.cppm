module;
#include <variant>
#include <QQmlEngine>
#include "Qcm/util/async.hpp"
#include "core/helper.h"

#include "Qcm/status/app_state.moc.h"
#ifdef Q_MOC_RUN
#include "Qcm/status/app_state.moc"
#endif

export module qcm.status.app_state;
export import qcm.core;

namespace qcm
{
export class AppState : public QObject {
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
module :private;

namespace qcm
{
AppState::AppState(QObject* parent): QObject(parent), m_state(Loading {}) {
    connect(this, &AppState::retry, this, &AppState::on_retry);
}
AppState::~AppState() {}

auto AppState::state() const -> const State& { return m_state; }
void AppState::set_state(const State& v) {
    if (v == m_state) {
        if (auto e = std::get_if<Error>(&m_state)) {
            e->err.append("\n");
            e->err.append(std::get_if<Error>(&v)->err);
            stateChanged();
            triggerSignal();
        }
        return;
    }
    m_state = v;
    stateChanged();
    triggerSignal();
}

void AppState::triggerSignal() {
    std::visit(overloaded { [this](const Loading& s) {
                               this->loading();
                           },
                            [this](const Welcome& s) {
                                this->welcome();
                            },
                            [this](const Main& s) {
                                this->main();
                            },
                            [this](const Error& e) {
                                this->error(e.err);
                            } },
               m_state);
}

void AppState::onQmlCompleted() {
    if (! is_state<Loading>()) {
        triggerSignal();
    }
}

void AppState::on_retry() {
    this->set_state(Loading {});

    QMetaObject::invokeMethod(
        this,
        [this] {
            disconnect(this, &AppState::retry, nullptr, nullptr);
            connect(this, &AppState::retry, this, &AppState::on_retry);
        },
        Qt::QueuedConnection);
}
}; // namespace qcm

#include "Qcm/status/app_state.moc.cpp"