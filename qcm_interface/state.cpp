#include "qcm_interface/state/app_state.h"
#include "core/core.h"
#include "core/variant_helper.h"

namespace qcm::state
{
AppState::AppState(QObject* parent)
    : QObject(parent), m_state(Loading {}), m_rescue(new QAsyncResult(this)) {}
AppState::~AppState() {}

auto AppState::rescue() const -> QAsyncResult* { return m_rescue; }
auto AppState::state() const -> const State& { return m_state; }
void AppState::set_state(const State& v) {
    if (v == m_state) return;

    stateChanged();
    std::visit(overloaded { [this](const Loading& s) {
                               m_state = s;
                           },
                            [this](const Start& s) {
                                m_state = s;
                                this->start();
                            },
                            [this](const Session& s) {
                                m_state = s;
                                this->session(s.session);
                            },
                            [this](const Error& e) {
                                m_state = e;
                                this->error(e.err);
                            } },
               v);

    if (! is_state<Error>()) {
        // clean callback
        this->rescue()->set_reload_callback({});
    }
}
}; // namespace qcm::state