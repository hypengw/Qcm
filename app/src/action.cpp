#include "Qcm/action.hpp"

#include <QSettings>
#include <QVariant>

#include "Qcm/app.hpp"
#include "Qcm/query/query.hpp"
#include "core/helper.h"

#include "core/asio/basic.h"
#include "core/qasio/qt_watcher.h"
#include "core/asio/watch_dog.h"

#include "Qcm/model/play_queue.hpp"
#include "Qcm/backend.hpp"
#include "Qcm/global.hpp"
#include "Qcm/util/global_static.hpp"

namespace qcm
{
auto Action::instance() -> Action* {
    static auto the =
        GlobalStatic::instance()->add<Action>("action", new Action(nullptr), [](Action* p) {
            delete p;
        });
    return the;
};

Action::Action(QObject* parent): QObject(parent) {}
Action::~Action() {}

Action* Action::create(QQmlEngine*, QJSEngine*) {
    auto act = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(act, QJSEngine::CppOwnership);
    return act;
}

void App::connect_actions() {
    connect(Action::instance(), &Action::queue, this, &App::on_queue);
    connect(Action::instance(), &Action::queue_next, this, &App::on_queue_next);
    connect(Action::instance(), &Action::switch_songs, this, &App::on_switch);
    connect(Action::instance(), &Action::play, this, &App::on_play);
    connect(Action::instance(), &Action::switch_queue, this, &App::on_switch_queue);
    connect(Action::instance(), &Action::record, this, &App::on_record);
    connect(Action::instance(), &Action::routeItem, this, &App::onRouteItem);

    connect(
        Action::instance(), &Action::next, this->playqueue(), QOverload<>::of(&PlayQueue::next));
    connect(
        Action::instance(), &Action::prev, this->playqueue(), QOverload<>::of(&PlayQueue::prev));

    connect(Action::instance(),
            &Action::record,
            this,
            [this,
             old_id        = rstd::Option<model::ItemId> {},
             old_source_id = rstd::Option<model::ItemId>()](enums::RecordAction act) mutable {
                switch (act) {
                case enums::RecordAction::RecordSwitch:
                case enums::RecordAction::RecordNext:
                case enums::RecordAction::RecordPrev: {
                    // TODO:
                    // if (old_id) {
                    //     Action::instance()->playback_log(enums::PlaybackState::StoppedState,
                    //                                     *old_id,
                    //                                     old_source_id.value_or(model::ItemId
                    //                                     {}));
                    // }
                    // old_id = this->playqueue()->currentId();
                    // auto source_id_var =
                    //     this->playqueue()->currentData(this->playqueue()->roleOf("sourceId"));
                    // if (auto source_id_p = get_if<model::ItemId>(&source_id_var)) {
                    //     old_source_id = *source_id_p;
                    // } else {
                    //     old_source_id.reset();
                    // }
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
                &Action::playback_log,
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
                    // auto session = Global::instance()->qsession();
                    // if (session->user()->userId().provider() == item.provider()) {
                    //     if (auto client = session->client()) {
                    //         client->api->play_state(*(client->instance),
                    //                                 state,
                    //                                 item,
                    //                                 source,
                    //                                 passed.count() / 1000.0,
                    //                                 extra);
                    //     }
                    // }
                });
    }

    {
        auto dog = make_rc<helper::WatchDog>();
        connect(Action::instance(), &Action::record, this, [dog, this](enums::RecordAction act) {
            switch (act) {
            case enums::RecordAction::RecordSwitchQueue:
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
            auto b = this->backend();
            Action::instance()->play_url(b->audio_url(*curId), true);
            /*
            QSettings s;
            auto      qu = s.value("play/play_quality").value<enums::AudioQuality>();

            auto hash    = song_uniq_hash(curId.value(), qu);
            auto path    = media_cache_path_of(hash);
            bool refresh = true;
            if (std::filesystem::exists(path)) {
                auto url = QUrl::fromLocalFile(convert_from<QString>(path.native()));
                Action::instance()->play(url, refresh);
                return;
            }

            auto ex = qcm::strand_executor();
            if (auto c = Global::instance()->qsession()->client()) {
                dog->spawn(
                    ex,
                    [c, curId, qu, hash, refresh] -> task<void> {
                        auto res = co_await c->api->media_url(*c->instance, curId.value(), qu);
                        res.transform([&hash, refresh](QUrl url) -> std::nullptr_t {
                               url = App::instance()->media_url(url, convert_from<QString>(hash));
                               Action::instance()->play(url, refresh);
                               return {};
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
                    */
        });
    }
}

void App::on_play(model::ItemId songId, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q   = App::instance()->play_id_queue();
    auto row = q->rowCount();
    q->insert(row, std::array { songId });
    if (sourceId.valid())
        App::instance()->playqueue()->updateSourceId(std::array { songId }, sourceId);
    q->setCurrentIndex(songId);
    Action::instance()->record(enums::RecordAction::RecordSwitch);
}

void App::on_queue_next(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q        = App::instance()->play_id_queue();
    auto idx      = std::min(q->currentIndex() + 1, q->rowCount());
    auto inserted = q->insert(idx, songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
    Action::instance()->toast(QString::fromStdString(
        inserted > 0 ? std::format("Add {} songs to queue", inserted) : "Already added"s));
}
void App::on_queue(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q        = App::instance()->play_id_queue();
    auto inserted = q->insert(q->rowCount(), songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
    Action::instance()->toast(QString::fromStdString(
        inserted > 0 ? std::format("Add {} songs to queue", inserted) : "Already added"s));
}
void App::on_switch(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q = App::instance()->play_id_queue();
    q->removeRows(0, q->rowCount());
    q->insert(q->rowCount(), songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
}

void App::on_record(enums::RecordAction) {}

void App::on_switch_queue(model::IdQueue* queue) {
    if (queue == nullptr) {
        INFO_LOG("queue is null");
    } else {
        this->playqueue()->setSourceModel(queue);
        if (queue->rowCount()) {
            queue->setCurrentIndex(0);
        }
        if (queue->rowCount() <= 1) {
            queue->requestNext();
        }
        Action::instance()->record(enums::RecordAction::RecordSwitchQueue);
    }
}

void App::onRouteItem(const model::ItemId& id, const QVariantMap& in_props) {
    Action::instance()->route(enums::SpecialRoute::SRMain);

    model::RouteMsg msg;
    auto            url = id.toPageUrl();

    auto props      = in_props;
    msg.dst         = url.toString();
    props["itemId"] = QVariant::fromValue(id);
    msg.props       = std::move(props);

    Action::instance()->route(QVariant::fromValue(msg));
}
} // namespace qcm

#include <Qcm/moc_action.cpp>