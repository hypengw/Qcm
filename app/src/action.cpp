#include "qcm_interface/action.h"
#include "Qcm/app.h"
#include "core/strv_helper.h"

namespace qcm
{

void App::connect_actions() {
    connect(Action::instance(), &Action::queue_songs, this, &App::on_queue_songs);

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
    p->appendList(f);
    Action::instance()->toast(QString::fromStdString(f.size() ? fmt::format("Add {} songs to queue", f.size()) : "Already added"s));
}
} // namespace qcm