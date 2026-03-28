module;
#include "Qcm/backend.moc.h"
#include "Qcm/status/provider.moc.h"

#include "core/log.h"
#undef assert
#include <rstd/macro.hpp>

module qcm;
import :status.provider;
import :status.process;
import :app;
import qcm.qt;
import qcm.log;

using namespace qcm;
using namespace Qt::Literals::StringLiterals;

#ifdef _WIN32
template<typename T>
using stream_type = asio::basic_stream_socket<asio::generic::stream_protocol, T>;
#else
template<typename T>
using stream_type = asio::posix::basic_stream_descriptor<T>;
#endif

namespace qcm
{

namespace detail
{
class BackendHelper {
public:
    template<typename CompletionToken>
    static auto async_send(Backend& backend, msg::QcmMessage&& msg, CompletionToken&& token) {
        using ret = void(asio::error_code, msg::QcmMessage);
        return asio::async_initiate<CompletionToken, ret>(
            [&](auto&& handler) {
                asio::dispatch(
                    backend.m_context->get_executor(),
                    [&backend, msg = std::move(msg), handler = std::move(handler)] mutable {
                        msg.setId_proto(backend.serial());
                        backend.m_handlers.insert_or_assign(
                            msg.id_proto(), cppstd::move_only_function<ret> { std::move(handler) });
                        auto bytes = msg.serialize(backend.m_serializer.get());
                        backend.m_client->send({ bytes.constData(), (std::size_t)bytes.size() });
                    });
            },
            token);
    }
};
} // namespace detail

Backend::Backend(rc<ncrequest::Session> session)
    : m_thread(Box<QThread>::make()),
      m_context(
          Box<QtExecutionContext>::make(m_thread.get(), (QEvent::Type)QEvent::registerEventType())),
      m_process(new QProcess()),
      m_client(Box<ncrequest::WebSocketClient>::make(
          ncrequest::event::create<stream_type>(m_context->get_executor()), None(),
          mem_mgr().backend_mem)),
      m_serializer(Box<QProtobufSerializer>::make()),
      m_session(session),
      m_serial(1), // start from 1, as 0 is none
      m_port(0) {
    m_process->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_process->setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedErrorChannel);
    m_client->set_on_error_callback([](auto err) {
        LOG_ERROR("{}", err);
    });
    m_client->set_on_connected_callback([this]() {
        Q_EMIT this->connected(m_port);
    });
    m_client->set_on_message_callback(
        [this, cache = make_rc<std::vector<byte>>()](std::span<const std::byte> bytes, bool last) {
            if (! last) {
                std::ranges::copy(bytes, std::back_inserter(*cache));
            } else {
                msg::QcmMessage msg;
                if (cache->empty()) {
                    msg.deserialize(m_serializer.get(), bytes);
                } else {
                    std::ranges::copy(bytes, std::back_inserter(*cache));
                    msg.deserialize(m_serializer.get(), *cache);
                    cache->clear();
                }
                if (msg.type() != msg::MessageTypeGadget::MessageType::PROVIDER_SYNC_STATUS_MSG) {
                    LOG_INFO("ws recv: {}", msg.type());
                }

                if (auto it = m_handlers.find(msg.id_proto()); it != m_handlers.end()) {
                    it->second(asio::error_code {}, std::move(msg));
                    m_handlers.erase(it);
                } else {
                    qcm::process_msg(std::move(msg));
                }
            }
        });
    // start thread
    {
        QObject::connect(m_thread.get(), &QThread::finished, m_process, &QObject::deleteLater);
        m_thread->start();
        bool ok = m_process->moveToThread(m_thread.get());
        debug_assert(ok);
    }
    // connect signal
    {
        connect(this, &Backend::started, this, &Backend::on_started);
        connect(this, &Backend::connected, this, &Backend::on_connected);
        connect(this, &Backend::error, this, &Backend::on_error);
        connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
            this->error(rstd::into(std::format("Backend exitd: {}", exitCode)));
        });
        connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
            this->error(rstd::into(std::format(
                "Backend {}", QMetaEnum::fromType<QProcess::ProcessError>().valueToKey((int)err))));
        });
    }
    {
        asio::post(m_context->get_executor(), [] {
            plt::set_thread_name("ws");
        });
    }
}

Backend::~Backend() {
    QMetaObject::invokeMethod(m_process, [self = m_process] {
        self->waitForFinished();
        LOG_WARN("backend stopped");
        self->thread()->quit();
    });
    m_client.reset();
    m_thread->wait();
}

auto Backend::start(QStringView exe_, QStringView data_dir_, QStringView cache_dir_) -> bool {
    m_exe       = exe_.toString();
    m_data_dir  = data_dir_.toString();
    m_cache_dir = cache_dir_.toString();

    {
        std::error_code ec;
        auto            path = cppstd::filesystem::path(m_exe.toStdString());

        if (! cppstd::filesystem::exists(path, ec)) {
            error(rstd::into(cppstd::format("Not found:\n {}", path.string())));
            return false;
        }
    }

    {
        struct State {
            QTextStream                           out { stdout, QIODevice::WriteOnly };
            rstd::Option<QMetaObject::Connection> cnn;
        };
        static auto state = rstd::rc::make_rc<State>();

        if (state->cnn) {
            QObject::disconnect(*(state->cnn));
        }

        state->cnn = Some(
            connect(m_process,
                    &QProcess::readyReadStandardOutput,
                    m_process,
                    [this, state = state] mutable {
                        m_process->setReadChannel(QProcess::ProcessChannel::StandardOutput);
                        if (m_process->canReadLine()) {
                            auto line = m_process->readLine();
                            auto doc  = QJsonDocument::fromJson(line);
                            if (auto jport = doc.object().value("port"); ! jport.isUndefined()) {
                                auto port = jport.toVariant().value<i32>();
                                LOG_INFO("backend port: {}", port);
                                Q_EMIT this->started(port);
                            } else {
                                this->error("Read port from backend failed");
                            }
                            QObject::disconnect(*(state->cnn));
                            state->cnn = Some(QObject::connect(
                                m_process,
                                &QProcess::readyReadStandardOutput,
                                m_process,
                                [p = m_process, state] mutable {
                                    p->setReadChannel(QProcess::ProcessChannel::StandardOutput);
                                    bool flush = p->canReadLine();
                                    state->out << p->readAllStandardOutput();
                                    if (flush) state->out.flush();
                                }));
                        }
                    }));
    }

    m_context->post([this, exe = m_exe, data_dir = m_data_dir, cache_dir = m_cache_dir] {
        LOG_DEBUG("starting backend: {}", exe);
        m_process->start(exe, { u"--data"_s, data_dir, u"--cache"_s, cache_dir });
    });
    return true;
}

void Backend::on_retry() { start(m_exe, m_data_dir, m_cache_dir); }

void Backend::on_error(QString) {
    m_context->post([this] {
        LOG_DEBUG("kill backend");
        m_process->kill();
    });
}

void Backend::on_started(i32 port) {
    m_port = port;
    m_client->connect(std::format("ws://127.0.0.1:{}", port));
}

void Backend::on_connected(i32) {
    auto msg = msg::QcmMessage();
    msg.setType(msg::MessageTypeGadget::MessageType::TEST_REQ);
    msg.setTestReq({});
    send_immediate(std::move(msg));
}

void Backend::send_immediate(msg::QcmMessage&& msg) {
    msg.setId_proto(serial());
    auto bytes = msg.serialize(m_serializer.get());
    m_client->send({ bytes.constData(), (std::size_t)bytes.size() });
}

auto Backend::send(msg::QcmMessage&& msg) -> task<Result<msg::QcmMessage, msg::Error>> {
    auto [ec, var] =
        co_await detail::BackendHelper::async_send(*this, std::move(msg), qcm::as_tuple(use_task));
    co_return Ok(var);
}

auto Backend::base() const -> std::string {
    return rstd::into(rstd::format("http://127.0.0.1:{}", m_port));
}

auto Backend::image(QStringView item_type, QStringView id, QStringView image_type)
    -> ncrequest::Request {
    auto url = rstd::format("{}/image/{}/{}/{}", this->base(), item_type, id, image_type);
    return ncrequest::Request { std::string_view(url) };
}
auto Backend::image(model::ItemId id, enums::ImageType image_type) -> ncrequest::Request {
    auto type = id.type();
    if (type == enums::ItemType::ItemAlbumArtist) type = enums::ItemType::ItemArtist;
    auto url = rstd::format("{}/image/{}/{}/{}", this->base(), type, id.id(), image_type);
    return ncrequest::Request { std::string_view(url) };
}

auto Backend::audio_url(model::ItemId id) -> QUrl {
    return rstd::into(rstd::format("{}/audio/{}/{}", this->base(), id.type(), id.id()));
}

auto Backend::serial() -> i32 {
    i32 cur = m_serial.load();
    for (;;) {
        const i32 to = (cur + 1) % std::numeric_limits<i32>::max();
        if (m_serial.compare_exchange_strong(cur, to)) {
            break;
        }
    }
    return cur;
}

} // namespace qcm

namespace qcm
{
ProviderMetaStatusModel::ProviderMetaStatusModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent) {}
ProviderMetaStatusModel::~ProviderMetaStatusModel() {}
ProviderStatusModel::ProviderStatusModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      m_syncing(false),
      m_lib_status(new LibraryStatus(this)) {
    connect(m_lib_status,
            &LibraryStatus::activedChanged,
            this,
            &ProviderStatusModel::libraryStatusChanged);
}
ProviderStatusModel::~ProviderStatusModel() {}

void ProviderStatusModel::updateSyncStatus(const msg::model::ProviderSyncStatus& s) {
    static auto role = Qt::UserRole + 1 +
                       msg::model::ProviderStatus::staticMetaObject.indexOfProperty("syncStatus");
    auto        id   = s.id_proto();
    if (auto v = this->query(id); v) {
        auto& value = *v;
        value.setSyncStatus(s);

        if (s.state() == msg::model::SyncStateGadget::SyncState::SYNC_STATE_SYNCING) {
            setSyncing(true);
        } else {
            checkSyncing();
        }

        if (auto idx = this->query_idx(id); idx) {
            auto qidx = this->index(*idx);
            dataChanged(qidx, qidx, { role });
        }
    }
}

auto ProviderStatusModel::metaById(const model::ItemId& item_id) const -> QVariant {
    auto p = this->query(item_id.id());
    if (p) {
        auto metas = App::instance()->provider_meta_status();
        if (auto m = metas->query(p->typeName())) {
            return QVariant::fromValue(*m);
        }
    }
    return {};
}

auto ProviderStatusModel::svg(qint32 idx) const -> QString {
    if (this->rowCount() > idx && idx >= 0) {
        auto& p     = this->at(idx);
        auto  metas = App::instance()->provider_meta_status();
        if (auto m = metas->query(p.typeName())) {
            return m->svg();
        }
    }
    return "";
}

auto ProviderStatusModel::svg(const model::ItemId& item_id) const -> QString {
    auto p = this->query(item_id.id());
    if (p) {
        auto metas = App::instance()->provider_meta_status();
        if (auto m = metas->query(p->typeName())) {
            return m->svg();
        }
    }
    return "";
}

QVariant ProviderStatusModel::itemById(const model::ItemId& item_id) const {
    auto p = this->query(item_id.id());
    if (p) return QVariant::fromValue(*p);
    return {};
}

auto ProviderStatusModel::syncing() const -> bool { return m_syncing; }

void ProviderStatusModel::setSyncing(bool v) {
    if (m_syncing != v) {
        m_syncing = v;
        syncingChanged(v);
    }
}
void ProviderStatusModel::checkSyncing() {
    bool out = false;
    for (auto i = 0; i < rowCount(); i++) {
        auto& p = this->at(i);
        if (p.syncStatus().state() == msg::model::SyncStateGadget::SyncState::SYNC_STATE_SYNCING) {
            out = true;
            break;
        }
    }
    setSyncing(out);
}

auto ProviderStatusModel::libraryStatus() const -> LibraryStatus* { return m_lib_status; }

static constexpr std::string_view inactived_library_key { "provider/inactived_libraries" };
LibraryStatus::LibraryStatus(QObject* parent): QObject(parent) {
    QSettings s;
    for (const auto& v : s.value(inactived_library_key).toStringList()) {
        m_inactived.insert(v.toLongLong());
    }

    connect(this, &LibraryStatus::activedChanged, this, &LibraryStatus::activedIdsChanged);
    connect(this, &LibraryStatus::activedChanged, this, [this](i64, bool) {
        QSettings   s;
        QStringList list;
        for (auto el : m_inactived) list.emplaceBack(QString::number(el));
        s.setValue(inactived_library_key, list);
    });
}
LibraryStatus::~LibraryStatus() {}

auto LibraryStatus::activedIds() -> const QtProtobuf::int64List& {
    m_ids.clear();
    auto p = App::instance()->provider_status();
    for (auto i = 0; i < p->rowCount(); i++) {
        auto& provider = p->at(i);
        for (auto& l : provider.libraries()) {
            auto id = l.libraryId();
            if (actived(id)) m_ids.emplaceBack(id);
        }
    }
    return m_ids;
}

bool LibraryStatus::actived(i64 id) const { return ! m_inactived.contains(id); }
void LibraryStatus::setActived(i64 id, bool v) {
    bool has = ! m_inactived.contains(id);
    if (has != v) {
        if (has) {
            m_inactived.insert(id);
        } else {
            m_inactived.erase(id);
        }
        activedChanged(id, v);
    }
}
} // namespace qcm

namespace qcm
{

namespace
{
auto get_hash(const model::ItemId& id) -> usize { return std::hash<model::ItemId> {}(id); }
} // namespace

PlayIdQueue::PlayIdQueue(QObject* parent): model::IdQueue(parent) { setOptions(Options(~0)); }
PlayIdQueue::~PlayIdQueue() {}

PlayIdProxyQueue::PlayIdProxyQueue(QObject* parent)
    : QIdentityProxyModel(parent), m_support_shuffle(true), m_current_index(-1), m_shuffle(true) {
    connect(this, &PlayIdProxyQueue::shuffleChanged, this, &PlayIdProxyQueue::reShuffle);
    connect(this, &PlayIdProxyQueue::layoutChanged, this, [this] {
        if (m_shuffle.hasBinding()) m_shuffle.notify();
    });
}

PlayIdProxyQueue::~PlayIdProxyQueue() {}
void PlayIdProxyQueue::setSourceModel(QAbstractItemModel* source_model) {
    auto old = sourceModel();
    if (old == source_model) return;

    QIdentityProxyModel::setSourceModel(source_model);
#ifdef _WIN32
    connect(source_model,
            SIGNAL(currentIndexChanged(qint32)),
            this,
            SLOT(setCurrentIndexFromSource(qint32)));
#else
    QBindable<qint32> source_idx(source_model, "currentIndex");
    m_current_index.setBinding([source_idx, this] {
        auto source = source_idx.value();
        return m_shuffle.value() && m_support_shuffle.value() ? mapFromSource(source) : source;
    });
#endif

    shuffleSync();

    auto options      = source_model->property("options").value<model::IdQueue::Options>();
    m_support_shuffle = options.testFlag(model::IdQueue::Option::SupportShuffle);
    if (old) {
        disconnect(
            old, &QAbstractItemModel::rowsInserted, this, &PlayIdProxyQueue::onSourceRowsInserted);
        disconnect(
            old, &QAbstractItemModel::rowsRemoved, this, &PlayIdProxyQueue::onSourceRowsRemoved);
        disconnect(old, &QAbstractItemModel::rowsMoved, this, &PlayIdProxyQueue::onSourceRowsMoved);
        disconnect(old,
                   &QAbstractItemModel::rowsAboutToBeInserted,
                   this,
                   &PlayIdProxyQueue::onSourceRowsAboutToBeInserted);
    }
    connect(source_model,
            &QAbstractItemModel::rowsInserted,
            this,
            &PlayIdProxyQueue::onSourceRowsInserted);
    connect(source_model,
            &QAbstractItemModel::rowsAboutToBeInserted,
            this,
            &PlayIdProxyQueue::onSourceRowsAboutToBeInserted);
    connect(source_model,
            &QAbstractItemModel::rowsRemoved,
            this,
            &PlayIdProxyQueue::onSourceRowsRemoved);
    connect(
        source_model, &QAbstractItemModel::rowsMoved, this, &PlayIdProxyQueue::onSourceRowsMoved);
}

auto PlayIdProxyQueue::shuffle() const -> bool { return m_shuffle.value(); }
void PlayIdProxyQueue::setShuffle(bool v) { m_shuffle = v; }
auto PlayIdProxyQueue::bindableShuffle() -> QBindable<bool> { return &m_shuffle; }
auto PlayIdProxyQueue::useShuffle() const -> bool { return m_support_shuffle && shuffle(); }

auto PlayIdProxyQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void PlayIdProxyQueue::setCurrentIndex(qint32 idx) {
    sourceModel()->setProperty("currentIndex", mapToSource(idx));
}
void PlayIdProxyQueue::setCurrentIndexFromSource(qint32 source) {
    m_current_index =
        m_shuffle.value() && m_support_shuffle.value() ? mapFromSource(source) : source;
}
auto PlayIdProxyQueue::bindableCurrentIndex() -> const QBindable<qint32> {
    return &m_current_index;
}
auto PlayIdProxyQueue::randomIndex() -> int {
    auto cur   = currentIndex();
    auto count = rowCount();
    if (count <= 1) return cur;

    int out = cur;
    while (out == cur) {
        out = Random::get(0, count - 1);
    }
    setCurrentIndex(out);
    return out;
}

auto PlayIdProxyQueue::mapToSource(const QModelIndex& proxy_index) const -> QModelIndex {
    if (! proxy_index.isValid()) return QModelIndex();
    return sourceModel()->index(mapToSource(proxy_index.row()), proxy_index.column());
}

auto PlayIdProxyQueue::mapFromSource(const QModelIndex& sourc_index) const -> QModelIndex {
    if (! sourc_index.isValid()) return QModelIndex();
    return index(mapFromSource(sourc_index.row()), sourc_index.column());
}

auto PlayIdProxyQueue::mapToSource(int row) const -> int {
    if (row < 0 || row >= (int)m_shuffle_list.size()) return -1;
    return useShuffle() ? m_shuffle_list[row] : row;
}

auto PlayIdProxyQueue::mapFromSource(int row) const -> int {
    int proxy_row { -1 };
    if (useShuffle()) {
        if (auto it = m_source_to_proxy.find(row); it != m_source_to_proxy.end()) {
            proxy_row = it->second;
        } else {
            proxy_row = -1;
        }
    } else {
        proxy_row = row;
    }
    return proxy_row;
}

void PlayIdProxyQueue::reShuffle() {
    layoutAboutToBeChanged();
    if (useShuffle()) {
        Random::shuffle(m_shuffle_list.begin(), m_shuffle_list.end());
        if (auto it = std::find(m_shuffle_list.begin(), m_shuffle_list.end(), 0);
            it != m_shuffle_list.end()) {
            std::swap(*it, m_shuffle_list.front());
        }
        refreshFromSource();
    }
    layoutChanged();
}

void PlayIdProxyQueue::shuffleSync() {
    layoutAboutToBeChanged();
    auto count = sourceModel()->rowCount();
    auto old   = (int)m_shuffle_list.size();
    if (old < count) {
        while ((int)m_shuffle_list.size() < count) m_shuffle_list.push_back(m_shuffle_list.size());

        auto cur = m_current_index.value();
        Random::shuffle(m_shuffle_list.begin() + cur + 1, m_shuffle_list.end());
        refreshFromSource();

    } else if (old > count) {
        for (int i = 0, k = 1; i < count; i++) {
            if (m_shuffle_list[i] >= count) {
                std::swap(m_shuffle_list[i], m_shuffle_list[old - k]);
                k++;
            }
        }
        m_shuffle_list.resize(count);
        refreshFromSource();

        // do not need check cur here
        // let upstream check later
        // if upstream no change, cur is ok
    }
    layoutChanged();
}

void PlayIdProxyQueue::refreshFromSource() {
    int rowCount = sourceModel()->rowCount();
    m_source_to_proxy.clear();
    for (int proxyRow = 0; proxyRow < rowCount; ++proxyRow) {
        int sourceRow                = m_shuffle_list[proxyRow];
        m_source_to_proxy[sourceRow] = proxyRow;
    }
}

void PlayIdProxyQueue::onSourceRowsInserted(const QModelIndex&, int, int) { shuffleSync(); }
void PlayIdProxyQueue::onSourceRowsRemoved(const QModelIndex&, int, int) { shuffleSync(); }
void PlayIdProxyQueue::onSourceRowsMoved(const QModelIndex&, int, int, const QModelIndex&, int) {
    shuffleSync();
}

void PlayIdProxyQueue::onSourceRowsAboutToBeInserted(const QModelIndex&, int, int) {}

PlayQueue::PlayQueue(QObject* parent)
    : QIdentityProxyModel(parent),
      m_proxy(new PlayIdProxyQueue(parent)),
      m_loop_mode(LoopMode::NoneLoop),
      m_can_next(false),
      m_can_prev(false),
      m_can_jump(true),
      m_can_user_remove(true),
      m_random_mode(false),
      m_changed_timer(new QTimer(this)) {
    updateRoleNames(qcm::model::Song::staticMetaObject, this);
    connect(this, &PlayQueue::currentIndexChanged, this, [this](qint32 idx) {
        setCurrentSong(idx);

        LOG_DEBUG("queue: current index changed to {}, id {}",
                  idx,
                  m_current_song.as_ref()
                      .and_then([](auto& el) -> rstd::Option<i64> {
                          return rstd::into(el.key());
                      })
                      .unwrap_or(-1));
    });
    connect(m_proxy, &PlayIdProxyQueue::currentIndexChanged, this, &PlayQueue::checkCanMove);
    connect(this, &PlayQueue::loopModeChanged, m_proxy, [this] {
        m_proxy->setShuffle(loopMode() == LoopMode::ShuffleLoop);
        checkCanMove();
    });
    connect(this, &PlayQueue::pendingIdsChanged, this, &PlayQueue::fetchSongs);
    connect(m_changed_timer, &QTimer::timeout, this, [this]() {
        auto source_model = sourceModel();
        if (! source_model) return;
        if (m_changed_ids.empty()) return;

        cppstd::unordered_set<model::ItemId> ids { m_changed_ids.begin(), m_changed_ids.end() };
        m_changed_ids.clear();
        cppstd::vector<qint32> rows_to_update;

        for (auto i = 0; i < rowCount(); i++) {
            if (auto id = getId(i); ids.contains(*id)) {
                rows_to_update.push_back(i);
            }
        }
        for (auto row : rows_to_update) {
            auto idx = index(row, 0);
            dataChanged(idx, idx);
            if (row == m_current_index) {
                setCurrentSong(currentIndex());
            }
        }
    });

    m_changed_timer->setSingleShot(true);
    m_changed_timer->setInterval(200);
    m_notify_handle =
        AppStore::instance()->songs.store_reg_notify([this](std::span<const i64> ids) {
            std::vector<model::ItemId> changed_ids;
            for (auto id : ids) {
                auto item_id = model::ItemId { enums::ItemType::ItemSong, id };
                changed_ids.push_back(item_id);
            }
            addChangedId(changed_ids);
        });

    loopModeChanged(m_loop_mode);
}
PlayQueue::~PlayQueue() {}
void PlayQueue::drop_global() {
    AppStore::instance()->songs.store_unreg_notify(m_notify_handle);
    m_songs.clear();
}

auto PlayQueue::roleNames() const -> QHash<int, QByteArray> { return roleNamesRef(); }
auto PlayQueue::data(const QModelIndex& index, int role) const -> QVariant {
    auto row = index.row();
    auto id  = getId(row);
    do {
        if (! id) break;
        auto it    = m_songs.find(*id);
        bool it_ok = it != m_songs.end();

        if (auto prop = this->propertyOfRole(role); prop) {
            if (it_ok) {
                auto* song = it->second.item();
                if (song) {
                    return prop.value().readOnGadget(song);
                } else {
                    return prop.value().readOnGadget(&m_placeholder);
                }
            } else if (prop->name() == "itemId"sv) {
                return QVariant::fromValue(*id);
            } else if (prop->name() == "sourceId"sv) {
                // if (auto it = m_source_map.find(hash); it != m_source_map.end()) {
                //     return QVariant::fromValue(it->second);
                // } else {
                //     return prop.value().readOnGadget(&m_placeholder);
                // }
            } else {
                return prop.value().readOnGadget(&m_placeholder);
            }
        }
    } while (0);
    return {};
}

void PlayQueue::setSourceModel(QAbstractItemModel* source_model) {
    auto old = sourceModel();

    if (old == source_model) return;

    QIdentityProxyModel::setSourceModel(source_model);
#ifdef _WIN32
    connect(source_model, SIGNAL(currentIndexChanged(qint32)), this, SLOT(setCurrentIndex(qint32)));
#else
    QBindable<qint32> source_idx(source_model, "currentIndex");
    m_current_index.setBinding([source_idx] {
        return source_idx.value();
    });
#endif

    if (old) {
        disconnect(old, &QAbstractItemModel::rowsInserted, this, &PlayQueue::onSourceRowsInserted);
        disconnect(old, &QAbstractItemModel::rowsRemoved, this, &PlayQueue::onSourceRowsRemoved);
        disconnect(old,
                   &QAbstractItemModel::rowsAboutToBeRemoved,
                   this,
                   &PlayQueue::onSourceRowsAboutToBeRemoved);
        disconnect(this, SIGNAL(requestNext()), old, SIGNAL(requestNext()));
    }
    connect(this, SIGNAL(requestNext()), source_model, SIGNAL(requestNext()));
    connect(
        source_model, &QAbstractItemModel::rowsInserted, this, &PlayQueue::onSourceRowsInserted);
    connect(source_model, &QAbstractItemModel::rowsRemoved, this, &PlayQueue::onSourceRowsRemoved);
    connect(source_model,
            &QAbstractItemModel::rowsAboutToBeRemoved,
            this,
            &PlayQueue::onSourceRowsAboutToBeRemoved);

    m_options = source_model->property("options").value<model::IdQueue::Options>();
    m_proxy->setSourceModel(source_model);
    checkCanMove();

    if (old) {
        this->onSourceRowsInserted({}, 0, source_model->rowCount());
    }

    if (auto p = source_model->property("name"); p.isValid()) {
        m_name = p.toString();
        nameChanged();
    }

    setCanJump(m_options & Option::SupportJump);
    setCanRemove(m_options & Option::SupportUserRemove);
}

auto PlayQueue::currentSong() const -> model::Song {
    if (m_current_song) {
        if (auto ptr = m_current_song->item()) {
            return *ptr;
        } else {
            LOG_ERROR("no such song: {}", m_current_song->key().value_or(-1));
        }
    }

    model::Song s {};
    if (auto id = currentId()) {
        s.setItemId(*id);
        if (auto it = m_source_map.find(*id); it != m_source_map.end()) {
            // s.sourceId = it->second;
        }
    }
    return s;
}

void PlayQueue::setCurrentSong(rstd::Option<SongItem> s) {
    if (m_current_song != s) {
        m_current_song = std::move(s);
        currentSongChanged();
    }
}
void PlayQueue::setCurrentSong(qint32 idx) {
    setCurrentSong(getId(idx).and_then([this](const auto& id) -> rstd::Option<SongItem> {
        if (auto it = m_songs.find(id); it != m_songs.end()) {
            return Some(SongItem { it->second });
        }
        return None();
    }));
}

auto PlayQueue::name() const -> const QString& { return m_name; }
auto PlayQueue::currentId() const -> rstd::Option<model::ItemId> { return getId(currentIndex()); }

auto PlayQueue::getId(qint32 idx) const -> rstd::Option<model::ItemId> {
    if (auto source = sourceModel()) {
        auto val = source->data(source->index(idx, 0));
        if (auto pval = get_if<model::ItemId>(&val)) {
            return Some(*pval);
        }
    }
    return None();
}

auto PlayQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void PlayQueue::setCurrentIndex(qint32 source) { m_current_index = source; }
auto PlayQueue::bindableCurrentIndex() -> const QBindable<qint32> { return &m_current_index; }

auto PlayQueue::currentData(int role) const -> QVariant {
    return data(index(currentIndex(), 0), role);
}

auto PlayQueue::loopMode() const noexcept -> enums::LoopMode { return m_loop_mode; }
void PlayQueue::setLoopMode(enums::LoopMode mode) {
    if (ycore::cmp_set(m_loop_mode, mode)) {
        loopModeChanged(m_loop_mode);
    }
}
void PlayQueue::iterLoopMode() {
    using M   = LoopMode;
    auto mode = loopMode();
    switch (mode) {
    case M::NoneLoop: mode = M::SingleLoop; break;
    case M::SingleLoop: mode = M::ListLoop; break;
    case M::ListLoop: mode = M::ShuffleLoop; break;
    case M::ShuffleLoop: mode = M::NoneLoop; break;
    }
    setLoopMode(mode);
}
auto PlayQueue::randomMode() const noexcept -> bool { return m_random_mode; }
void PlayQueue::setRandomMode(bool v) {
    if (ycore::cmp_set(m_random_mode, v)) {
        randomModeChanged(m_random_mode);
    }
}

auto PlayQueue::canNext() const noexcept -> bool { return m_can_next; }
auto PlayQueue::canPrev() const noexcept -> bool { return m_can_prev; }
auto PlayQueue::canJump() const noexcept -> bool { return m_can_jump; }
auto PlayQueue::canRemove() const noexcept -> bool { return m_can_user_remove; }

void PlayQueue::setCanNext(bool v) {
    if (ycore::cmp_set(m_can_next, v)) {
        canNextChanged();
    }
}
void PlayQueue::setCanPrev(bool v) {
    if (ycore::cmp_set(m_can_prev, v)) {
        canPrevChanged();
    }
}
void PlayQueue::setCanJump(bool v) {
    if (ycore::cmp_set(m_can_jump, v)) {
        canJumpChanged();
    }
}
void PlayQueue::setCanRemove(bool v) {
    if (ycore::cmp_set(m_can_user_remove, v)) {
        canRemoveChanged();
    }
}

void PlayQueue::next() {
    auto mode = loopMode();
    if (mode == LoopMode::SingleLoop) mode = LoopMode::ListLoop;
    next(mode);
}
void PlayQueue::prev() {
    auto mode = loopMode();
    if (mode == LoopMode::SingleLoop) mode = LoopMode::ListLoop;
    prev(mode);
}
void PlayQueue::next(LoopMode mode) {
    bool support_loop = m_options.testFlag(Option::SupportLoop);
    auto count        = m_proxy->rowCount();
    if (count == 0) return;
    auto cur = m_proxy->currentIndex();

    switch (mode) {
    case LoopMode::NoneLoop: {
        if (cur + 1 < count) {
            m_proxy->setCurrentIndex(cur + 1);
        } else {
            return;
        }
        break;
    }
    case LoopMode::ListLoop: {
        m_proxy->setCurrentIndex((cur + 1) % count);
        break;
    }
    case LoopMode::ShuffleLoop: {
        if (m_random_mode) {
            m_proxy->randomIndex();
        } else {
            m_proxy->setCurrentIndex((cur + 1) % count);
        }
        break;
    }
    case LoopMode::SingleLoop: {
    }
    }
    requestNext();
    Action::instance()->record(enums::RecordAction::RecordNext);
}
void PlayQueue::prev(LoopMode mode) {
    bool support_loop = m_options.testFlag(Option::SupportLoop);
    auto count        = m_proxy->rowCount();
    if (count == 0) return;
    auto cur = m_proxy->currentIndex();

    switch (mode) {
    case LoopMode::NoneLoop: {
        if (cur >= 1) {
            m_proxy->setCurrentIndex(cur - 1);
        } else {
            return;
        }
        break;
    }
    case LoopMode::ListLoop:
    case LoopMode::ShuffleLoop: {
        m_proxy->setCurrentIndex(cur <= 0 ? std::max(count, 1) - 1 : cur - 1);
        break;
    }
    case LoopMode::SingleLoop: {
    }
    }
    Action::instance()->record(enums::RecordAction::RecordPrev);
}

void PlayQueue::startIfNoCurrent() {
    if (rowCount() > 0 && currentIndex() < 0) {
        m_proxy->setCurrentIndex(0);
        Action::instance()->record(enums::RecordAction::RecordSwitch);
    }
}

void PlayQueue::clear() { removeRows(0, rowCount()); }

auto PlayQueue::update(std::span<const model::Song> in) -> void {
    auto store = AppStore::instance();
    for (auto& el : in) {
        auto item = store->songs.store_item(el.id_proto());
        if (item) {
            m_songs.insert_or_assign(el.itemId(), *item);
        }
    }
}

void PlayQueue::updateSourceId(std::span<const model::ItemId> songIds,
                               const model::ItemId&           sourceId) {
    for (auto& el : songIds) {
        if (auto it = m_songs.find(el); it != m_songs.end()) {
            // TODO
            // it->second.sourceId = sourceId;
        } else {
            m_source_map.insert_or_assign(el, sourceId);
        }
    }
}

void PlayQueue::onSourceRowsInserted(const QModelIndex&, int first, int last) {
    auto store = AppStore::instance();
    for (int i = first; i <= last; i++) {
        if (auto id = getId(i)) {
            if (m_songs.contains(*id)) continue;
            if (auto song_item = store->songs.store_item(id->id())) {
                m_songs.insert_or_assign(*id, *song_item);
            } else {
                m_pending_ids.insert(*id);
            }
        }
    }
    checkCanMove();
    pendingIdsChanged();
}

void PlayQueue::onSourceRowsAboutToBeRemoved(const QModelIndex&, int first, int last) {
    for (int i = first; i <= last; i++) {
        auto id = getId(i);
        if (id) {
            m_songs.erase(*id);
            m_source_map.erase(*id);
        }
    }
}
void PlayQueue::onSourceRowsRemoved(const QModelIndex&, int, int) { checkCanMove(); }
void PlayQueue::checkCanMove() {
    auto count              = rowCount();
    bool support_prev       = m_options.testFlag(Option::SupportPrev);
    bool support_loop       = m_options.testFlag(Option::SupportLoop);
    auto check_on_none_loop = [this, support_prev, count] {
        setCanPrev(m_proxy->currentIndex() > 0 && support_prev && count);
        setCanNext(count > m_proxy->currentIndex() + 1 && count);
    };
    switch (m_loop_mode) {
    case LoopMode::NoneLoop: {
        check_on_none_loop();
        break;
    }
    case LoopMode::ShuffleLoop:
    case LoopMode::ListLoop: {
        if (support_loop) {
            setCanPrev(support_prev && count);
            setCanNext(count);
        } else {
            check_on_none_loop();
        }
        break;
    }
    default: {
        setCanPrev(support_prev && count);
        setCanNext(count);
    }
    }
}
bool PlayQueue::move(qint32 src, qint32 dst, qint32 count) {
    auto parent = this->index(-1, 0);
    return moveRows(parent, src, count, parent, dst);
}

void PlayQueue::addChangedId(std::span<const model::ItemId> ids) {
    m_changed_ids.insert(ids.begin(), ids.end());
    m_changed_timer->start();
}
void PlayQueue::fetchSongs() {
    if (m_pending_ids.empty()) return;

    auto ex      = asio::make_strand(qcm::pool_executor());
    auto backend = App::instance()->backend();

    auto req  = msg::GetSongsByIdReq {};
    auto view = cppstd::views::transform(m_pending_ids, [](const auto& id) {
        return id.id();
    });
    req.setIds({ view.begin(), view.end() });
    m_pending_ids.clear();

    auto self = QWatcher { this };
    asio::co_spawn(
        ex,
        [self, backend, req = std::move(req)] mutable -> task<void> {
            auto rsp = co_await backend->send(std::move(req));
            co_await qcm::qexecutor_switch();

            if (rsp) {
                std::vector<model::ItemId> fetched_ids;
                std::vector<i64>           id_list;

                auto store  = AppStore::instance();
                auto handle = self->m_notify_handle;
                for (auto& song_proto : rsp->items()) {
                    auto song = model::Song { song_proto };
                    fetched_ids.push_back(song.itemId());
                    id_list.push_back(song.id_proto());
                    auto [item, _] = store->songs.store_insert(song);
                    self->m_songs.insert_or_assign(song.itemId(), item);
                }
                for (qsizetype i = 0; i < rsp->extras().size(); i++) {
                    auto id = rsp->items().at(i).id_proto();
                    merge_store_extra(store->songs, id, rsp->extras().at(i));
                }
                store->songs.store_changed_callback(id_list, handle);
                self->addChangedId(fetched_ids);
            }

            co_return;
        },
        asio_detached_log_t {});
}

} // namespace qcm

#include "Qcm/status/provider.moc.cpp"
#include "Qcm/backend.moc"
