#include "qcm_interface/action.h"
#include "Qcm/app.h"
#include "core/strv_helper.h"

#include "asio_helper/detached_log.h"

namespace qcm
{

void App::connect_actions() {
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
                    auto client = Global::instance()->get_client(item.provider().toStdString());
                    if (client) {
                        client->api->play_state(
                            client->instance, state, item, source, passed.count() / 1000.0, extra);
                    }
                });
    }
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
                if (auto c = Global::instance()->client(user->userId().provider().toStdString())) {
                    co_await c.api->logout(c.instance);

                    QMetaObject::invokeMethod(
                        Global::instance()->user_model(), &UserModel::check_user, user);
                }
            }
            co_return;
        },
        helper::asio_detached_log);
}
} // namespace qcm