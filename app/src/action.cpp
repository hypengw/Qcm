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
#include "Qcm/backend_msg.hpp"
#include "Qcm/backend.hpp"
#include "Qcm/global.hpp"
#include "Qcm/player.hpp"
#include "Qcm/util/global_static.hpp"
#include "Qcm/status/provider_status.hpp"

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

void App::connect_components() {
    connect(provider_status(),
            &ProviderStatusModel::libraryStatusChanged,
            this,
            &App::libraryStatusChanged);
}

void App::connect_actions() {
    connect(Action::instance(), &Action::queue, this, &App::onQueue);
    connect(Action::instance(), &Action::queue_next, this, &App::onQueueNext);
    connect(Action::instance(), &Action::switch_songs, this, &App::onSwitch);
    connect(Action::instance(), &Action::play, this, &App::onPlay);
    connect(Action::instance(), &Action::switch_queue, this, &App::onSwitchQueue);
    connect(Action::instance(), &Action::record, this, &App::onRecord);
    connect(Action::instance(), &Action::routeItem, this, &App::onRouteItem);

    connect(
        Action::instance(), &Action::next, this->playqueue(), QOverload<>::of(&PlayQueue::next));
    connect(
        Action::instance(), &Action::prev, this->playqueue(), QOverload<>::of(&PlayQueue::prev));

    // player
    {
        const auto player = global()->player();
        connect(Action::instance(),
                &Action::playUrl,
                player,
                [p = player](const QUrl& url, bool reload) {
                    if (reload && p->source() == url) {
                        p->reset_source();
                    }
                    p->set_source(url);

                    if (url.isValid()) p->play();
                });

        connect(Action::instance(), &Action::toggle, player, &Player::toggle);

        connect(
            player,
            &Player::playbackStateChanged,
            player,
            [p = player](Player::PlaybackState, Player::PlaybackState new_) {
                auto queue = App::instance()->playqueue();
                if (new_ == Player::PlaybackState::StoppedState && p->source().isValid()) {
                    if (p->duration() > 0 && p->position() / (double)p->duration() > 0.98) {
                        queue->next(queue->loopMode());
                    }
                }

                if (new_ == Player::PlaybackState::PlayingState) {
                    auto cur = queue->currentId().unwrap_or(model::ItemId {});
                    Action::instance()->playLog(
                        (qint32)msg::model::PlaylogActionGadget::PlaylogAction::PLAYLOG_ACTION_PLAY,
                        cur,
                        {});
                }
            });
    }

    // stop logger
    connect(
        Action::instance(),
        &Action::record,
        this,
        [this,
         old_id        = rstd::Option<model::ItemId> {},
         old_source_id = rstd::Option<model::ItemId>()](enums::RecordAction act) mutable {
            switch (act) {
            case enums::RecordAction::RecordSwitch:
            case enums::RecordAction::RecordNext:
            case enums::RecordAction::RecordPrev: {
                if (old_id) {
                    Action::instance()->playLog(
                        (qint32)msg::model::PlaylogActionGadget::PlaylogAction::PLAYLOG_ACTION_STOP,
                        *old_id,
                        old_source_id.unwrap_or(model::ItemId {}));
                }
                old_id = this->playqueue()->currentId();
                auto source_id_var =
                    this->playqueue()->currentData(this->playqueue()->roleOf("sourceId"));
                if (auto source_id_p = get_if<model::ItemId>(&source_id_var)) {
                    old_source_id = rstd::Some(*source_id_p);
                } else {
                    old_source_id = rstd::None();
                }
                break;
            }
            default: {
            }
            }
        });

    {
        using namespace std::chrono;
        struct Timer {
            time_point<system_clock> point;
            milliseconds             passed;
        };

        connect(Action::instance(),
                &Action::playLog,
                this,
                [this, t = make_rc<Timer>()](
                    qint32 act_int, model::ItemId song_id, model::ItemId source) {
                    using PlaylogAction = msg::model::PlaylogActionGadget::PlaylogAction;
                    const auto act      = (PlaylogAction)act_int;
                    auto       backend  = this->backend();
                    auto       req      = msg::PlaylogReq {};
                    const auto now      = system_clock::now();

                    req.setSongId(song_id.id());
                    req.setAction(act);
                    req.setTimestamp(duration_cast<milliseconds>(now.time_since_epoch()).count());
                    req.setSourceId(source.id());
                    req.setSourceType((msg::model::ItemTypeGadget::ItemType)source.type());

                    asio::co_spawn(
                        qcm::strand_executor(),
                        [backend, req] mutable -> task<void> {
                            co_await backend->send(std::move(req));
                        },
                        helper::asio_detached_log_t {});
                });
    }

    connect(Action::instance(), &Action::record, this, [this](enums::RecordAction act) {
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

        auto curId = playqueue()->currentId();
        if (! curId || ! curId->valid()) return;
        auto b = this->backend();
        Action::instance()->playUrl(b->audio_url(*curId), true);
    });

    connect(Action::instance(), &Action::scheduleRenderJob, qApp, [](QRunnable* job, qint32 stage) {
        if (QWindowList wins = qApp->allWindows(); ! wins.isEmpty()) {
            if (auto win = qobject_cast<QQuickWindow*>(wins.front())) {
                win->scheduleRenderJob(job, (QQuickWindow::RenderStage)stage);
            }
        }
    });
}

void App::onPlay(model::ItemId songId, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q   = App::instance()->play_id_queue();
    auto row = q->rowCount();
    q->insert(row, std::array { songId });
    if (sourceId.valid())
        App::instance()->playqueue()->updateSourceId(std::array { songId }, sourceId);
    q->setCurrentIndex(songId);
    Action::instance()->record(enums::RecordAction::RecordSwitch);
}

void App::onQueueNext(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
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
void App::onQueue(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
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
void App::onSwitch(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
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

void App::onRecord(enums::RecordAction) {}

void App::onSwitchQueue(model::IdQueue* queue) {
    if (queue == nullptr) {
        LOG_INFO("queue is null");
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