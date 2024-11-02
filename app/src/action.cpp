#include "qcm_interface/action.h"

#include <QSettings>
#include <QVariant>

#include "Qcm/app.h"
#include "Qcm/query/query.h"
#include "core/strv_helper.h"

#include "asio_helper/basic.h"
#include "asio_qt/qt_watcher.h"
#include "qcm_interface/plugin.h"
#include "asio_helper/watch_dog.h"

namespace qcm
{

void App::connect_actions() {
    connect(Action::instance(), &Action::load_session, this, &App::on_load_session);
    connect(Action::instance(), &Action::switch_user, this, &App::on_switch_user);
    connect(Action::instance(), &Action::logout, this, &App::on_logout);
    connect(Action::instance(), &Action::collect, this, &App::on_collect);
    connect(Action::instance(), &Action::sync_collection, this, &App::on_sync_collecttion);
    connect(Action::instance(), &Action::queue_ids, this, &App::on_queue_ids);
    connect(Action::instance(), &Action::switch_ids, this, &App::on_switch_ids);
    connect(Action::instance(), &Action::play_by_id, this, &App::on_play_by_id);
    connect(Action::instance(), &Action::record, this, &App::on_record);
    connect(Action::instance(),
            QOverload<const query::Song&>::of(&Action::play),
            this,
            &App::on_play_song);
    connect(Action::instance(), &Action::queue, this, &App::on_queue);
    connect(Action::instance(), &Action::switch_to, this, &App::on_switch_to);

    connect(Global::instance(), &Global::sessionChanged, Global::instance(), &Global::save_user);

    connect(
        Action::instance(), &Action::next, this->playqueue(), QOverload<>::of(&PlayQueue::next));
    connect(
        Action::instance(), &Action::prev, this->playqueue(), QOverload<>::of(&PlayQueue::prev));

    connect(Action::instance(),
            &Action::record,
            this,
            [this,
             old_id        = std::optional<model::ItemId> {},
             old_source_id = std::optional<model::ItemId>()](enums::RecordAction act) mutable {
                switch (act) {
                case enums::RecordAction::RecordSwitch:
                case enums::RecordAction::RecordNext:
                case enums::RecordAction::RecordPrev: {
                    if (old_id) {
                        Action::instance()->playbackLog(enums::PlaybackState::StoppedState,
                                                        *old_id,
                                                        old_source_id.value_or(model::ItemId {}));
                    }
                    old_id             = m_playqueu->currentId();
                    auto source_id_var = m_playqueu->currentData(m_playqueu->roleOf("sourceId"));
                    if (auto source_id_p = get_if<model::ItemId>(&source_id_var)) {
                        old_source_id = *source_id_p;
                    } else {
                        old_source_id.reset();
                    }
                    break;
                }
                default: {
                }
                }
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

    {
        auto dog = make_rc<helper::WatchDog>();
        connect(Action::instance(), &Action::record, this, [dog, this](enums::RecordAction act) {
            switch (act) {
            case enums::RecordAction::RecordSwitch:
            case enums::RecordAction::RecordNext:
            case enums::RecordAction::RecordPrev: {
                break;
            }
            default: {
                return;
            }
            }

            dog->cancel();
            auto curId = playqueue()->currentId();
            if (! curId || ! curId->valid()) return;
            QSettings s;
            auto      qu = s.value("play/play_quality").value<enums::AudioQuality>();

            auto hash    = song_uniq_hash(curId.value(), qu);
            auto path    = media_cache_path_of(hash);
            bool refresh = true;
            if (std::filesystem::exists(path)) {
                auto url = QUrl::fromLocalFile(convert_from<QString>(path.native()));
                Global::instance()->action()->play(url, refresh);
                return;
            }

            auto ex = asio::make_strand(Global::instance()->pool_executor());
            if (auto c = Global::instance()->qsession()->client()) {
                dog->spawn(
                    ex,
                    [c, curId, qu, hash, refresh] -> task<void> {
                        auto res = co_await c->api->media_url(*c->instance, curId.value(), qu);
                        res.transform([&hash, refresh](QUrl url) -> bool {
                               url = App::instance()->media_url(url, convert_from<QString>(hash));
                               Global::instance()->action()->play(url, refresh);
                               return true;
                           })
                            .transform_error([](auto err) -> std::nullptr_t {
                                Global::instance()->errorOccurred(
                                    convert_from<QString>(err.what()));
                                return {};
                            });
                        co_return;
                    },
                    helper::asio_detached_log_t {});
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
        [client, weak] -> task<void> {
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
        helper::asio_detached_log_t {});
}
void App::on_play_by_id(model::ItemId songId, model::ItemId sourceId) {
    auto q   = App::instance()->play_id_queue();
    auto row = q->rowCount();
    q->insert(row, std::array { songId });
    if (sourceId.valid())
        App::instance()->playqueue()->updateSourceId(std::array { songId }, sourceId);
    q->setCurrentIndex(songId);
    Action::instance()->record(enums::RecordAction::RecordSwitch);
}

void App::on_queue_ids(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    auto q        = App::instance()->play_id_queue();
    auto inserted = q->insert(q->rowCount(), songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
    Action::instance()->toast(QString::fromStdString(
        inserted > 0 ? fmt::format("Add {} songs to queue", inserted) : "Already added"s));
}
void App::on_switch_ids(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    auto q = App::instance()->play_id_queue();
    q->removeRows(0, q->rowCount());
    q->insert(q->rowCount(), songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
}

void App::on_logout() {
    auto ex = asio::make_strand(m_global->pool_executor());
    asio::co_spawn(
        ex,
        [] -> task<void> {
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
        helper::asio_detached_log_t {});
}

void App::on_switch_user(model::ItemId id) {
    auto user = Global::instance()->user_model()->find_by_url(id.toUrl());
    if (user != nullptr) {
        Global::instance()->switch_user(user);
    } else {
        ERROR_LOG("user not found");
    }
}

void App::on_collect(model::ItemId id, bool act) {
    auto ex = asio::make_strand(m_global->pool_executor());
    asio::co_spawn(
        ex,
        [id, act] -> task<void> {
            auto user = Global::instance()->qsession()->user();
            if (auto c = Global::instance()->qsession()->client()) {
                auto sql = Global::instance()->get_collection_sql();
                auto ok  = co_await c->api->collect(*(c->instance), id, act);
                if (ok.value_or(false)) {
                    co_await asio::post(
                        asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));

                    auto item = db::ColletionSqlBase::Item::from(user->userId(), id);
                    if (act) {
                        user->insert(id);
                        App::instance()->collected(id, true);
                        co_await sql->insert(std::array { item });
                    } else {
                        user->remove(id);
                        App::instance()->collected(id, false);
                        co_await sql->remove(user->userId(), id);
                    }
                }
            }
            co_return;
        },
        helper::asio_detached_log_t {});
}

void App::on_sync_collecttion(enums::CollectionType ct) {
    auto ex = asio::make_strand(m_global->pool_executor());
    asio::co_spawn(
        ex,
        [ct] -> task<void> {
            co_await query::SyncAPi::sync_collection(ct);
        },
        helper::asio_detached_log_t {});
}

void App::on_record(enums::RecordAction record) {
    switch (record) {
    case enums::RecordAction::RecordSwitch: {
    }
    case enums::RecordAction::RecordNext: {
    }
    case enums::RecordAction::RecordPrev: {
    }
    }
}
void App::on_play_song(const query::Song& s) {
    App::instance()->playqueue()->update(std::array { s });
    on_play_by_id(s.id, {});
}
void App::on_queue(const std::vector<query::Song>& s) {
    std::vector<model::ItemId> ids;
    for (auto& el : s) {
        ids.emplace_back(el.id);
    }
    App::instance()->playqueue()->update(s);
    on_queue_ids(ids, {});
}
void App::on_switch_to(const std::vector<query::Song>& s) {
    std::vector<model::ItemId> ids;
    for (auto& el : s) {
        ids.emplace_back(el.id);
    }
    App::instance()->playqueue()->update(s);
    on_switch_ids(ids, {});
}

} // namespace qcm