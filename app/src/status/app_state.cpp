#include "Qcm/status/app_state.hpp"
#include "core/helper.h"

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

#include <Qcm/status/moc_app_state.cpp>