#include "qcm_interface/action.h"
#include "Qcm/app.h"
#include "core/strv_helper.h"

#include "asio_helper/detached_log.h"
#include "asio_qt/qt_watcher.h"
#include "qcm_interface/plugin.h"

namespace qcm
{

void App::connect_actions() {
    connect(Action::instance(), &Action::load_session, this, &App::on_load_session);
    connect(Action::instance(), &Action::switch_user, this, &App::on_switch_user);
    connect(Action::instance(), &Action::queue_songs, this, &App::on_queue_songs);
    connect(Action::instance(), &Action::logout, this, &App::on_logout);

    connect(Global::instance(), &Global::sessionChanged, Global::instance(), &Global::save_user);

    connect(this->playlist(), &Playlist::curChanged, this, [this, old = model::Song()]() mutable {
        std::optional<model::ItemId> old_id;
        if (auto itemId = meta_model::readOnGadget(old.source, "itemId"); itemId.isValid()) {
            old_id = itemId.value<model::ItemId>();
        }
        Action::instance()->playbackLog(
            enums::PlaybackState::StoppedState, old.id, old_id.value_or(model::ItemId {}));
        old = playlist()->cur();
    });

    {
        struct Timer {
            std::chrono::time_point<std::chrono::steady_clock> point;
            std::chrono::milliseconds                          passed;
        };

        auto t = make_rc<Timer>();

        connect(Action::instance(),
                &Action::playbackLog,
                this,
                [t](enums::PlaybackState state,
                    model::ItemId        item,
                    model::ItemId        source,
                    QVariantMap          extra) {
                    std::chrono::milliseconds passed;
                    if (state == enums::PlaybackState::PlayingState) {
                        t->point = std::chrono::steady_clock::now();
                    } else if (state == enums::PlaybackState::PausedState) {
                        t->passed += std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t->point);
                        t->point = std::chrono::steady_clock::now();
                    } else {
                        passed = t->passed + std::chrono::duration_cast<std::chrono::milliseconds>(
                                                 std::chrono::steady_clock::now() - t->point);
                        t->passed = {};
                    }
                    auto session = Global::instance()->qsession();
                    if (session->user()->userId().provider() == item.provider()) {
                        if (auto client = session->client()) {
                            client->api->play_state(*(client->instance),
                                                    state,
                                                    item,
                                                    source,
                                                    passed.count() / 1000.0,
                                                    extra);
                        }
                    }
                });
    }
}

void App::on_load_session(model::Session* session) {
    if (session == nullptr) {
        return;
    }
    auto client = session->client();
    if (! client) {
        return;
    }

    auto weak = helper::QWatcher { session };
    if (weak->parent() == nullptr) {
        weak.take_owner();
    }
    auto ex = asio::make_strand(m_global->pool_executor());
    asio::co_spawn(
        ex,
        [client, weak] -> asio::awaitable<void> {
            auto res = co_await client->api->session_check(*(client->instance), weak);
            auto g   = Global::instance();
            co_await asio::post(asio::bind_executor(g->qexecutor(), asio::use_awaitable));

            res.transform([&weak, g](bool ok) {
                   if (ok) {
                       if (weak) {
                           weak->set_valid(true);
                           g->set_session(weak.get());
                           g->app_state()->set_state(state::AppState::Session { weak.get() });
                       }
                   } else {
                       if (g->app_state()->is_state<state::AppState::Loading>()) {
                           g->app_state()->set_state(state::AppState::Start {});
                       }
                       Action::instance()->toast("session not valid");
                   }
               })
                .transform_error([&weak, g](const auto& err) {
                    auto state = state::AppState::Error { convert_from<QString>(err.what()) };
                    if (! g->app_state()->is_state<state::AppState::Session>()) {
                        if (weak) {
                            state.fatal = false;
                            weak.take_owner();
                            g->app_state()->set_state(state);
                            g->app_state()->rescue()->set_reload_callback([weak] {
                                Action::instance()->load_session(weak.get());
                            });
                        } else {
                            state.fatal = true;
                            g->app_state()->set_state(state);
                        }
                    } else {
                        Action::instance()->toast(state.err);
                    }
                    DEBUG_LOG("{}", err);
                });
            co_return;
        },
        helper::asio_detached_log);
}

void App::on_queue_songs(const std::vector<model::Song>& songs) {
    auto                     p    = App::instance()->playlist();
    auto                     view = std::ranges::filter_view(songs, [](const model::Song& s) {
        return s.canPlay;
    });
    std::vector<model::Song> f { view.begin(), view.end() };
    auto                     size = p->appendList(f);
    Action::instance()->toast(QString::fromStdString(
        size ? fmt::format("Add {} songs to queue", size) : "Already added"s));
}
void App::on_logout() {
    auto ex = asio::make_strand(m_global->pool_executor());
    asio::co_spawn(
        ex,
        [] -> asio::awaitable<void> {
            if (auto user = Global::instance()->qsession()->user()) {
                if (auto c = Global::instance()->qsession()->client()) {
                    co_await c->api->logout(*(c->instance));

                    auto session = Global::instance()->qsession();
                    Global::instance()->app_state()->set_state(state::AppState::Start {});
                    Global::instance()->user_model()->delete_user(session->user()->userId());
                    Global::instance()->set_session(nullptr);
                }
            }
            co_return;
        },
        helper::asio_detached_log);
}

void App::on_switch_user(model::ItemId id) {
    auto user = Global::instance()->user_model()->find_by_url(id.toUrl());
    if (user != nullptr) {
        Global::instance()->switch_user(user);
    } else {
        ERROR_LOG("user not found");
    }
}
} // namespace qcm