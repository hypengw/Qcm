#include "service_qml_ncm/client.h"

#include <mutex>
#include <ranges>
#include <unordered_set>
#include <QUuid>
#include "asio_helper/basic.h"
#include "core/str_helper.h"
#include "core/strv_helper.h"
#include "core/qstr_helper.h"

#include "qcm_interface/async.inl"
#include "qcm_interface/global.h"
#include "qcm_interface/model/session.h"
#include "qcm_interface/oper/album_oper.h"
#include "qcm_interface/oper/artist_oper.h"
#include "qcm_interface/oper/song_oper.h"
#include "qcm_interface/oper/mix_oper.h"
#include "qcm_interface/oper/radio_oper.h"
#include "qcm_interface/oper/program_oper.h"
#include "qcm_interface/sql/item_sql.h"
#include "qcm_interface/notifier.h"
#include "qcm_interface/action.h"

#include "service_qml_ncm/model.h"
#include "asio_helper/detached_log.h"
#include "asio_qt/qt_holder.h"

#include "ncm/api/feedback_weblog.h"
#include "ncm/api/user_account.h"
#include "ncm/api/logout.h"
#include "ncm/api/song_like.h"
#include "ncm/api/radio_like.h"
#include "ncm/api/song_url.h"
#include "ncm/api/album_sublist.h"
#include "ncm/api/artist_sublist.h"
#include "ncm/api/user_playlist.h"
#include "ncm/api/djradio_sublist.h"
#include "ncm/api/v1_album.h"
#include "ncm/api/playlist_detail.h"
#include "ncm/api/artist.h"
#include "ncm/api/artist_albums.h"
#include "ncm/api/artist_songs.h"
#include "ncm/api/song_detail.h"
#include "ncm/api/djradio_detail.h"
#include "ncm/api/djradio_program.h"
#include "ncm/api/comments.h"
#include "ncm/api/album_sub.h"
#include "ncm/api/playlist_subscribe.h"
#include "ncm/api/djradio_sub.h"
#include "ncm/api/artist_sub.h"
#include "ncm/api/playlist_create.h"
#include "ncm/api/playlist_delete.h"
#include "ncm/api/playlist_manipulate_tracks.h"
#include "ncm/api/playlist_update_name.h"

#include "ncm/client.h"

namespace ncm::impl
{
using ClientBase = qcm::ClientBase;

struct Client : public ClientBase {
    Client(ncm::Client ncm): ncm(ncm), user_fav_mix_id() {}
    ncm::Client           ncm;
    ItemId                user_id;
    std::optional<ItemId> user_fav_mix_id;

    std::mutex mutex;

    auto rc() -> rc<Client> { return std::static_pointer_cast<Client>(shared_from_this()); }
};
static auto get_client(ClientBase& c) -> ncm::Client* { return &static_cast<Client&>(c).ncm; }
static auto get_client_rc(ClientBase& c) -> rc<ncm::impl::Client> {
    return static_cast<Client&>(c).rc();
}
static auto get_user_id(ClientBase& cb) -> ItemId {
    auto&            c = static_cast<Client&>(cb);
    std::unique_lock lock { c.mutex };
    return c.user_id;
}
static auto get_user_fav_mix_id(ClientBase& cb) -> std::optional<ItemId> {
    auto&            c = static_cast<Client&>(cb);
    std::unique_lock lock { c.mutex };
    return c.user_fav_mix_id;
}
static void set_user_id(ClientBase& cb, const ItemId& id) {
    auto&            c = static_cast<Client&>(cb);
    std::unique_lock lock { c.mutex };
    c.user_id = id;
}
static void set_user_fav_mix_id(ClientBase& cb, const ItemId& id) {
    auto&            c = static_cast<Client&>(cb);
    std::unique_lock lock { c.mutex };
    c.user_fav_mix_id = id;
}
} // namespace ncm::impl
namespace
{

constexpr int        MIN_IMG_SIZE { 300 };
constexpr std::array IMG_DL_SIZES { 240, 480, 960, 1920 };

inline QSize get_down_size(const QSize& req) {
    usize req_size = std::sqrt(req.width() * req.height());
    auto  it       = std::lower_bound(IMG_DL_SIZES.begin(), IMG_DL_SIZES.end(), req_size);
    usize size     = it != IMG_DL_SIZES.end() ? *it : IMG_DL_SIZES.back();

    return req.scaled(size, size, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
}

auto prepare_session(ncm::Client c, qcm::model::ItemId userId) -> qcm::task<void> {
    auto sql = qcm::Global::instance()->get_collection_sql();

    ncm::api::SongLike api;
    api.input.uid                       = convert_from<ncm::model::UserId>(userId);
    auto                            res = co_await c.perform(api);
    std::vector<qcm::model::ItemId> ids;
    if (res) {
        for (auto& id : res.value().ids) {
            auto item_id = ncm::to_ncm_id(ncm::model::IdType::Song, id);
            ids.push_back(item_id);
        }

        co_await sql->refresh(userId, "song", ids);

        qcm::Global::instance()->qsession()->user()->query();
    }
    co_return;
}

template<typename T>
auto insert_artist(const T& in_list, rc<qcm::db::ItemSqlBase> sql, std::set<std::string> on_update)
    -> qcm::task<void> {
    auto list = qcm::oper::ArtistOper::create_list(0);
    for (auto& ar : in_list) {
        auto oper = qcm::oper::ArtistOper(list.emplace_back());
        convert(oper, ar);
    }
    co_await sql->insert(list, on_update);
}

template<typename T, typename Func>
auto insert_album(const T& in_list, Func&& get_artists, rc<qcm::db::ItemSqlBase> sql,
                  const std::set<std::string>& columns, const std::set<std::string>& artist_columns)
    -> qcm::task<void> {
    auto                                                list = qcm::oper::AlbumOper::create_list(0);
    std::vector<qcm::db::ItemSqlBase::RelationInsertId> album_artist_ids;
    for (auto& el : in_list) {
        auto oper = qcm::oper::AlbumOper(list.emplace_back());
        convert(oper, el);
        for (auto& ar : get_artists(el)) {
            album_artist_ids.emplace_back(oper.libraryId(),
                                          convert_from<ncm::ItemId>(el.id),
                                          convert_from<ncm::ItemId>(ar.id));
        }
    }
    co_await sql->insert(list, columns);

    co_await insert_artist(
        std::ranges::join_view(std::ranges::transform_view(in_list, get_artists)),
        sql,
        artist_columns);

    co_await sql->insert_album_artist(album_artist_ids);
}

template<typename T>
auto insert_song(const T& in_list, rc<qcm::db::ItemSqlBase> sql,
                 const std::set<std::string>& on_update) -> qcm::task<void> {
    auto                                      list = qcm::oper::SongOper::create_list(0);
    std::vector<qcm::db::ItemSqlBase::IdPair> song_artist_ids;
    for (const auto& el : in_list) {
        auto oper = qcm::oper::SongOper(list.emplace_back());
        convert(oper, el);
    }
    co_await sql->insert(list, on_update);
}

template<typename T, typename Func>
auto insert_song(const T& in_list, Func&& get_artists, rc<qcm::db::ItemSqlBase> sql,
                 const std::set<std::string>& on_update,
                 const std::set<std::string>& on_artist_update) -> qcm::task<void> {
    auto                                                list = qcm::oper::SongOper::create_list(0);
    std::vector<qcm::db::ItemSqlBase::RelationInsertId> song_artist_ids;
    for (const auto& el : in_list) {
        auto oper = qcm::oper::SongOper(list.emplace_back());
        convert(oper, el);
        for (auto& ar : get_artists(el)) {
            song_artist_ids.emplace_back(oper.libraryId(),
                                         convert_from<ncm::ItemId>(el.id),
                                         convert_from<ncm::ItemId>(ar.id));
        }
    }
    co_await sql->insert(list, on_update);

    co_await insert_artist(
        std::ranges::join_view(std::ranges::transform_view(in_list, get_artists)),
        sql,
        on_artist_update);

    co_await sql->insert_song_artist(song_artist_ids);
}

template<typename T, typename FuncAlbum, typename FuncArtist>
auto insert_song_album(const T& in_list, FuncAlbum&& get_album, FuncArtist&& get_artists,
                       rc<qcm::db::ItemSqlBase> sql, const std::set<std::string>& on_update,
                       const std::set<std::string>& on_album_update,
                       const std::set<std::string>& on_artist_update) -> qcm::task<void> {
    auto list       = qcm::oper::SongOper::create_list(0);
    auto album_list = qcm::oper::AlbumOper::create_list(0);
    std::vector<qcm::db::ItemSqlBase::RelationInsertId> song_artist_ids;
    for (const auto& el : in_list) {
        auto oper = qcm::oper::SongOper(list.emplace_back());
        convert(oper, el);
        for (const auto& ar : get_artists(el)) {
            song_artist_ids.emplace_back(oper.libraryId(),
                                         convert_from<ncm::ItemId>(el.id),
                                         convert_from<ncm::ItemId>(ar.id));
        }

        const auto& al      = get_album(el);
        auto        al_oper = qcm::oper::AlbumOper(album_list.emplace_back());
        convert(al_oper, al);
    }
    co_await sql->insert(list, on_update);
    co_await sql->insert(album_list, on_album_update);

    co_await insert_artist(
        std::ranges::join_view(std::ranges::transform_view(in_list, get_artists)),
        sql,
        on_artist_update);

    co_await sql->insert_song_artist(song_artist_ids);
}

template<typename T>
auto insert_djraio(const T& in_list, rc<qcm::db::ItemSqlBase> sql,
                   const std::set<std::string>& on_update) -> qcm::task<void> {
    auto list = qcm::oper::DjradioOper::create_list(0);
    for (const auto& el : in_list) {
        auto oper = qcm::oper::DjradioOper(list.emplace_back());
        convert(oper, el);
    }
    co_await sql->insert(list, on_update);
}

template<typename T>
auto insert_program(const T& in_list, rc<qcm::db::ItemSqlBase> sql,
                    const std::set<std::string>& on_update) -> qcm::task<void> {
    auto list = qcm::oper::ProgramOper::create_list(0);
    for (const auto& el : in_list) {
        auto oper = qcm::oper::ProgramOper(list.emplace_back());
        convert(oper, el);
    }
    co_await sql->insert(list, on_update);
}

void refresh_fav_mix(rc<ncm::impl::Client> c, i32 lib_id,
                     std::optional<qcm::model::ItemId> song_id = {}, bool collect_act = false) {
    auto ex = asio::make_strand(qcm::pool_executor());
    asio::co_spawn(
        ex,
        [c, ex, lib_id, song_id, collect_act] -> qcm::task<void> {
            auto timer = make_rc<asio::steady_timer>(ex);
            timer->expires_after(asio::chrono::milliseconds(1000));
            {
                auto sql    = qcm::Global::instance()->get_item_sql();
                auto mix_id = get_user_fav_mix_id(*c);
                if (! mix_id) {
                    if (auto mix = co_await sql->select_mix(get_user_id(*c), 5)) {
                        mix_id = mix->id;
                        set_user_fav_mix_id(*c, mix->id);
                    }
                }

                if (mix_id && song_id) {
                    if (collect_act) {
                        co_await sql->insert_mix_song(lib_id, 0, *mix_id, std::array { *song_id });
                    } else {
                        co_await sql->remove_mix_song(lib_id, *mix_id, std::array { *song_id });
                    }
                }
            }

            co_await timer->async_wait(asio::as_tuple(asio::use_awaitable));

            auto                   sql    = qcm::Global::instance()->get_item_sql();
            auto                   userId = get_user_id(*c);
            ncm::api::UserPlaylist api;
            convert(api.input.uid, userId);
            api.input.limit = 1;
            auto out        = co_await c->ncm.perform(api);
            if (out) {
                auto list = qcm::oper::MixOper::create_list(0);
                for (auto& el : out->playlist) {
                    auto oper = qcm::oper::MixOper(list.emplace_back());
                    convert(oper, el);
                }
                co_await sql->insert(list, {});

                if (list.size()) {
                    qcm::Notifier::instance()->itemChanged(qcm::oper::MixOper(list.at(0)).id());
                }
            }
        },
        helper::asio_detached_log_t {});
}

} // namespace

namespace ncm::impl
{

static auto server_url(ClientBase& cbase, const qcm::model::ItemId& id) -> std::string {
    auto c = get_client(cbase);
    switch (UNWRAP(ncm_id_type(id))) {
    case ncm::model::IdType::Song: {
        return fmt::format("{}/#song?id={}", ncm::BASE_URL, id.id());
    }
    default: {
        return {};
    }
    }
}

static void play_state(ClientBase& cbase, qcm::enums::PlaybackState state,
                       qcm::model::ItemId itemId, qcm::model::ItemId sourceId, i64 played_second,
                       QVariantMap) {
    if (state == qcm::enums::PlaybackState::PausedState) return;
    auto c         = *get_client(cbase);
    auto state_old = c.prop("play_state");
    if (state == qcm::enums::PlaybackState::PlayingState) {
        if (state_old) {
            if (std::any_cast<qcm::enums::PlaybackState>(state_old.value()) == state) {
                return;
            }
        }
    }
    c.set_prop("play_state", state);

    auto                     ex = asio::make_strand(c.get_executor());
    ncm::api::FeedbackWeblog api;

    if (! helper::variant_convert(api.input.id, to_ncm_id(itemId))) {
        ERROR_LOG("");
        return;
    }

    api.input.act  = state == qcm::enums::PlaybackState::PlayingState
                         ? ncm::params::FeedbackWeblog::Action::Start
                         : ncm::params::FeedbackWeblog::Action::End;
    api.input.time = played_second;

    helper::variant_convert(api.input.sourceId, to_ncm_id(sourceId));

    asio::co_spawn(
        ex,
        [c, api]() mutable -> qcm::task<void> {
            co_await c.perform(api);
            co_return;
        },
        helper::asio_detached_log_t {});
}

static auto router(ClientBase& cbase) -> rc<qcm::Router> {
    auto c          = *get_client(cbase);
    auto router_any = c.prop("router"sv);
    if (! router_any) {
        c.set_prop("router"sv, make_rc<qcm::Router>());
        router_any = c.prop("router"sv);
    }
    auto router = std::any_cast<rc<qcm::Router>>(router_any);
    return router;
}

static auto logout(ClientBase& cbase) -> qcm::task<void> {
    auto             c = *get_client(cbase);
    ncm::api::Logout api;
    co_await c.perform(api);
    co_return;
}
static auto session_check(ClientBase& cbase, helper::QWatcher<qcm::model::Session> session)
    -> qcm::task<Result<bool>> {
    auto                  c = *get_client(cbase);
    ncm::api::UserAccount api;
    auto                  ex  = co_await asio::this_coro::executor;
    auto                  out = co_await c.perform(api);
    co_await asio::post(asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));

    auto user = session->user();
    co_return out.transform([user, ex, &session, c, &cbase](const auto& out) -> bool {
        if (out.profile) {
            session->set_provider(convert_from<QString>(ncm::provider));
            user->set_userId(convert_from<ItemId>(out.profile->userId));
            set_user_id(cbase, user->userId());
            user->set_nickname(convert_from<QString>(out.profile->nickname));
            user->set_avatarUrl(convert_from<QString>(out.profile->avatarUrl));
            user->query();
            asio::co_spawn(ex, prepare_session(c, user->userId()), helper::asio_detached_log_t {});
            return true;
        }
        return false;
    });
}
void save(ClientBase& cbase, const std::filesystem::path& path) {
    auto c = *get_client(cbase);
    c.save(path);
}
void load(ClientBase& cbase, const std::filesystem::path& path) {
    auto c = *get_client(cbase);
    c.load(path);
}

bool make_request(ClientBase& cbase, request::Request& req, const QUrl& url,
                  const qcm::Client::ReqInfo& info) {
    auto c = *get_client(cbase);
    std::visit(overloaded { [c, &url, &req](const qcm::Client::ReqInfoImg& info) {
                   request::UrlParams query;
                   if (info.size.isValid()) {
                       auto down_size = get_down_size(info.size);
                       query.set_param("param",
                                       fmt::format("{}y{}", down_size.width(), down_size.height()));
                   }
                   req = c.make_req<ncm::api::CryptoType::NONE>(
                       convert_from<std::string>(url.toString()), query);
               } },
               info);
    return true;
}

auto collect(ClientBase& cbase, qcm::model::ItemId item_id, bool act) -> qcm::task<Result<bool>> {
    using Res = qcm::task<Result<bool>>;
    auto c    = *get_client(cbase);
    auto nid  = to_ncm_id(item_id);

    co_return co_await std::visit(
        overloaded { [act, &c](model::PlaylistId id) -> Res {
                        ncm::api::PlaylistSubscribe api;
                        api.input.sub = act;
                        api.input.id  = id;
                        auto ok       = co_await c.perform(api);
                        co_return ok.transform([](const auto& out) {
                            return out.code == 200;
                        });
                    },
                     [act, &c](model::ArtistId id) -> Res {
                         ncm::api::ArtistSub api;
                         api.input.sub = act;
                         api.input.id  = id;
                         auto ok       = co_await c.perform(api);
                         co_return ok.transform([](const auto& out) {
                             return out.code == 200;
                         });
                     },
                     [act, &c](model::AlbumId id) -> Res {
                         ncm::api::AlbumSub api;
                         api.input.sub = act;
                         api.input.id  = id;
                         auto ok       = co_await c.perform(api);
                         co_return ok.transform([](const auto& out) {
                             return out.code == 200;
                         });
                     },
                     [act, &c](model::DjradioId id) -> Res {
                         ncm::api::DjradioSub api;
                         api.input.sub = act;
                         api.input.id  = id;
                         auto ok       = co_await c.perform(api);
                         co_return ok.transform([](const auto& out) {
                             return out.code == 200;
                         });
                     },
                     [act, &c, &cbase, &item_id](model::SongId id) -> Res {
                         ncm::api::RadioLike api;
                         api.input.like    = act;
                         api.input.trackId = id;
                         auto ok           = co_await c.perform(api);

                         co_return ok.transform([&cbase, &item_id, act](const auto& out) {
                             auto ok = out.code == 200;
                             if (ok) {
                                 // TODO: lib_id
                                 // notify user fav playlist changed
                                 refresh_fav_mix(get_client_rc(cbase), 0, item_id, act);
                             }
                             return ok;
                         });
                     },
                     [](auto) -> Res {
                         co_return false;
                     } },
        nid);
}

auto media_url(ClientBase& cbase, qcm::model::ItemId id, qcm::enums::AudioQuality quality)
    -> qcm::task<Result<QUrl>> {
    auto c = *get_client(cbase);

    ncm::api::SongUrl api;
    api.input.ids.push_back(convert_from<model::SongId>(id));
    api.input.level = (ncm::params::SongUrl::Level)quality;
    auto res        = co_await c.perform(api);
    co_return res.transform([](const auto& su) -> QUrl {
        if (su.data.size()) {
            return QString::fromStdString(su.data.at(0).url);
        }
        return {};
    });
}

auto sync_collection(ClientBase& cbase, qcm::enums::CollectionType collection_type)
    -> qcm::task<Result<bool>> {
    auto c       = *get_client(cbase);
    auto user_id = get_user_id(cbase);

    auto collect_sql = qcm::Global::instance()->get_collection_sql();
    auto item_sql    = qcm::Global::instance()->get_item_sql();

    switch (collection_type) {
    case qcm::enums::CollectionType::CTAlbum: {
        ncm::api::AlbumSublist api;
        api.input.limit                 = 1000;
        bool                   has_more = false;
        std::vector<ItemId>    collects;
        std::vector<QDateTime> collect_times;
        auto                   type = convert_from<QString>(collection_type);
        do {
            auto out = co_await c.perform(api);

            api.input.offset += api.input.limit;
            if (out) {
                has_more = out->hasMore;
                {
                    for (usize i = 0; i < out->data.size(); i++) {
                        auto& el = out->data[i];
                        collects.push_back(convert_from<ItemId>(el.id));
                        collect_times.push_back(convert_from<QDateTime>(el.subTime));
                    }

                    co_await insert_album(
                        out->data,
                        [](const auto& el) -> const decltype(out->data[0].artists)& {
                            return el.artists;
                        },
                        item_sql,
                        { "name", "picUrl", "trackCount" },
                        { "name" });
                }
            } else {
                co_return nstd::unexpected(out.error());
            }
        } while (has_more || api.input.offset >= std::numeric_limits<i32>::max());
        co_await collect_sql->refresh(user_id, type, collects, collect_times);
        break;
    }
    case qcm::enums::CollectionType::CTArtist: {
        ncm::api::ArtistSublist api;
        api.input.limit                 = 1000;
        bool                   has_more = false;
        std::vector<ItemId>    collects;
        std::vector<QDateTime> collect_times;
        auto                   type = convert_from<QString>(collection_type);
        auto                   cur  = QDateTime::currentDateTimeUtc();
        do {
            auto out = co_await c.perform(api);
            api.input.offset += api.input.limit;
            if (out) {
                has_more = out->hasMore;
                {
                    auto list = qcm::oper::ArtistOper::create_list(out->data.size());
                    for (usize i = 0; i < out->data.size(); i++) {
                        auto& el = out->data[i];
                        collects.push_back(convert_from<ItemId>(el.id));
                        collect_times.push_back(cur);
                        auto oper = qcm::oper::ArtistOper(list[i]);
                        convert(oper, el);

                        cur = cur.addSecs(-5);
                    }
                    co_await item_sql->insert(list, { "name", "picUrl", "albumCount" });
                }
            } else {
                co_return nstd::unexpected(out.error());
            }
        } while (has_more || api.input.offset >= std::numeric_limits<i32>::max());
        co_await collect_sql->refresh(user_id, type, collects, collect_times);
        break;
    }
    case qcm::enums::CollectionType::CTPlaylist: {
        ncm::api::UserPlaylist api;
        convert(api.input.uid, user_id);
        api.input.limit                 = 1000;
        bool                   has_more = false;
        std::vector<ItemId>    collects;
        std::vector<QDateTime> collect_times;
        auto                   type = convert_from<QString>(collection_type);
        auto                   cur  = QDateTime::currentDateTimeUtc();
        do {
            auto out = co_await c.perform(api);
            api.input.offset += api.input.limit;
            if (out) {
                has_more = out->more;
                {
                    auto list = qcm::oper::MixOper::create_list(out->playlist.size());
                    for (usize i = 0; i < out->playlist.size(); i++) {
                        auto& el = out->playlist[i];
                        collects.push_back(convert_from<ItemId>(el.id));
                        collect_times.push_back(cur);
                        auto oper = qcm::oper::MixOper(list[i]);
                        convert(oper, el);
                        cur = cur.addSecs(-5);
                    }
                    co_await item_sql->insert(list, {});
                }
            } else {
                co_return nstd::unexpected(out.error());
            }
        } while (has_more || api.input.offset >= std::numeric_limits<i32>::max());
        co_await collect_sql->refresh(user_id, type, collects, collect_times);
        break;
    }
    case qcm::enums::CollectionType::CTRadio: {
        ncm::api::DjradioSublist api;
        api.input.limit                 = 1000;
        bool                   has_more = false;
        std::vector<ItemId>    collects;
        std::vector<QDateTime> collect_times;
        auto                   type = convert_from<QString>(collection_type);
        auto                   cur  = QDateTime::currentDateTimeUtc();
        do {
            auto out = co_await c.perform(api);
            api.input.offset += api.input.limit;
            if (out) {
                has_more = out->hasMore;
                {
                    auto list = qcm::oper::DjradioOper::create_list(out->djRadios.size());
                    for (usize i = 0; i < out->djRadios.size(); i++) {
                        auto& el = out->djRadios[i];
                        collects.push_back(convert_from<ItemId>(el.id));
                        collect_times.push_back(convert_from<QDateTime>(el.createTime));
                        auto oper = qcm::oper::DjradioOper(list[i]);
                        convert(oper, el);
                    }
                    co_await item_sql->insert(list, {});
                }
            } else {
                co_return nstd::unexpected(out.error());
            }
        } while (has_more || api.input.offset >= std::numeric_limits<i32>::max());
        co_await collect_sql->refresh(user_id, type, collects, collect_times);
    }
    }
    co_return true;
}

auto sync_items(ClientBase& cbase, std::span<const qcm::model::ItemId> itemIds)
    -> qcm::task<Result<bool>> {
    auto c   = *get_client(cbase);
    auto sql = qcm::Global::instance()->get_item_sql();
    if (itemIds.empty()) co_return false;
    auto type = ncm_id_type(itemIds.front());
    if (! type) co_return false;

    switch (type.value()) {
    case ncm::model::IdType::Album: {
        ncm::api::AlbumDetail     api;
        std::vector<model::Album> albums;
        std::vector<model::Song>  songs;
        for (auto& el : itemIds) {
            convert(api.input.id, el);
            auto out = co_await c.perform(api);
            if (out) {
                albums.push_back(out->album);
                songs.insert(songs.end(), out->songs.begin(), out->songs.end());
            } else {
                co_return nstd::unexpected(out.error());
            }
        }
        co_await insert_album(albums,
                              [](const auto& el) -> const decltype(albums[0].artists)& {
                                  return el.artists;
                              },
                              sql,
                              {},
                              { "name", "picUrl" });
        co_await insert_song(songs,
                             [](const auto& el) -> const decltype(songs[0].ar)& {
                                 return el.ar;
                             },
                             sql,
                             {},
                             { "name" });
        co_return true;
        break;
    }
    case ncm::model::IdType::Artist: {
        ncm::api::Artist api;
        for (auto& el : itemIds) {
            convert(api.input.id, el);
            auto out = co_await c.perform(api);
            if (out) {
                auto                  list = qcm::oper::ArtistOper::create_list(1);
                qcm::oper::ArtistOper oper(list.at(0));
                convert(oper, out->artist);
                co_await sql->insert(list, {});
            }
            if (! out) {
                co_return nstd::unexpected(out.error());
            }
        }
        break;
    }
    case ncm::model::IdType::Playlist: {
        ncm::api::PlaylistDetail api;
        for (auto& el : itemIds) {
            convert(api.input.id, el);
            api.input.n = 100000;
            auto out    = co_await c.perform(api);
            if (out) {
                auto               list = qcm::oper::MixOper::create_list(1);
                qcm::oper::MixOper oper(list.at(0));
                convert(oper, out->playlist);
                co_await sql->insert(list, {});

                std::vector<model::Song> songs;
                if (out->playlist.tracks) {
                    for (auto& el : out->playlist.tracks.value()) {
                        auto& s = songs.emplace_back();
                        convert(s, el);
                    }
                }

                co_await insert_song_album(
                    songs,
                    [](const auto& el) -> const decltype(songs[0].al)& {
                        return el.al;
                    },
                    [](const auto& el) -> const decltype(songs[0].ar)& {
                        return el.ar;
                    },
                    sql,
                    {},
                    { "name" },
                    { "name" });

                auto ids_view      = std::views::transform(songs, [](auto& el) {
                    ItemId id;
                    convert(id, el.id);
                    return id;
                });
                auto ids_hash_view = std::views::transform(ids_view, std::hash<ItemId>());

                std::unordered_set<usize> id_set(ids_hash_view.begin(), ids_hash_view.end());
                std::vector<ItemId>       ids;
                if (out->playlist.trackIds) {
                    auto ids_view =
                        std::views::transform(out->playlist.trackIds.value(), [](auto& el) {
                            ItemId id;
                            convert(id, el.id);
                            return id;
                        });
                    std::ranges::copy(ids_view, std::back_inserter(ids));

                    auto                filter = std::views::filter(ids, [&id_set](const auto& el) {
                        return ! id_set.contains(std::hash<ItemId>()(el));
                    });
                    std::vector<ItemId> songs(filter.begin(), filter.end());
                    co_await sync_items(cbase, songs);
                } else {
                    std::ranges::copy(ids_view, std::back_inserter(ids));
                }

                // TODO: lib_id
                co_await sql->refresh_mix_song(0, -1, oper.id(), ids);
            } else {
                co_return nstd::unexpected(out.error());
            }
        }
        break;
    }
    case ncm::model::IdType::Djradio: {
        ncm::api::DjradioDetail           api;
        ncm::api::DjradioProgram          program_api;
        std::vector<model::DjradioDetail> djradios;
        std::vector<model::Program>       programs;
        for (auto& el : itemIds) {
            convert(api.input.id, el);
            convert(program_api.input.radioId, el);
            program_api.input.limit = 100000;
            {
                auto out = co_await c.perform(api);
                if (out) {
                    djradios.push_back(out->data);
                } else {
                    co_return nstd::unexpected(out.error());
                }
            }
            {
                auto out = co_await c.perform(program_api);
                if (out) {
                    for (auto& el : out->programs) {
                        programs.emplace_back(el);
                    }
                } else {
                    co_return nstd::unexpected(out.error());
                }
            }
        }
        co_await insert_djraio(djradios, sql, {});
        co_await insert_program(programs, sql, {});
        co_await insert_song(std::views::transform(programs,
                                                   [](const auto& el) -> model::SongB {
                                                       auto s     = el.mainSong;
                                                       s.coverUrl = el.coverUrl;
                                                       return s;
                                                   }),
                             sql,
                             {});

        break;
    }
    case ncm::model::IdType::Song: {
        ncm::api::SongDetail api;
        for (auto& el : itemIds) {
            auto& id = api.input.ids.emplace_back();
            convert(id, el);
        }

        auto out = co_await c.perform(api);
        if (out) {
            co_await insert_song_album(
                out->songs,
                [](const auto& el) -> const decltype(out->songs[0].al)& {
                    return el.al;
                },
                [](const auto& el) -> const decltype(out->songs[0].ar)& {
                    return el.ar;
                },
                sql,
                {},
                { "name", "picUrl" },
                { "name" });
        } else {
            co_return nstd::unexpected(out.error());
        }
        break;
    }
    default: {
    }
    }
    co_return false;
}

auto sync_list(ClientBase& cbase, qcm::enums::SyncListType type, qcm::model::ItemId itemId,
               i32 offset, i32 limit) -> qcm::task<Result<i32>> {
    auto c   = *get_client(cbase);
    auto sql = qcm::Global::instance()->get_item_sql();
    switch (type) {
    case qcm::enums::SyncListType::CTArtistAlbum: {
        ncm::api::ArtistAlbums api;
        convert(api.input.id, itemId);
        api.input.offset = offset;
        api.input.limit  = limit;
        auto out         = co_await c.perform(api);
        if (out) {
            co_await insert_album(out->hotAlbums,
                                  [](const auto& el) -> const decltype(out->hotAlbums[0].artists)& {
                                      return el.artists;
                                  },
                                  sql,
                                  {},
                                  { "name" });
        }
        co_return out.transform([](auto& out) {
            return out.hotAlbums.size();
        });
        break;
    }
    case qcm::enums::SyncListType::CTArtistSong: {
        ncm::api::ArtistSongs api;
        convert(api.input.id, itemId);
        api.input.offset = offset;
        api.input.limit  = limit;
        auto out         = co_await c.perform(api);
        if (out) {
            co_await insert_song_album(
                out->songs,
                [](const auto& el) -> const decltype(out->songs[0].al)& {
                    return el.al;
                },
                [](const auto& el) -> const decltype(out->songs[0].ar)& {
                    return el.ar;
                },
                sql,
                {},
                { "name" },
                { "name" });
        }
        co_return out.transform([](auto& out) {
            return out.songs.size();
        });
        break;
    }
    }
    co_return 0;
}
auto comments(ClientBase& cbase, qcm::model::ItemId item_id, i32 offset, i32 limit, i32& total)
    -> qcm::task<Result<qcm::oper::OperList<qcm::model::Comment>>> {
    auto               c = *get_client(cbase);
    ncm::api::Comments api;

    using CT         = ncm::params::Comments::Type;
    using IT         = ncm::model::IdType;
    auto& id         = api.input.id;
    auto& type       = api.input.type;
    api.input.offset = offset;
    api.input.limit  = limit;
    id.id            = item_id.id().toStdString();
    switch (UNWRAP(ncm::ncm_id_type(item_id))) {
    case IT::Album: {
        type = CT::Album;
        break;
    }
    case IT::Song: {
        type = CT::Song;
        break;
    }
    case IT::Playlist: {
        type = CT::Playlist;
        break;
    }
    case IT::Program: {
        type = CT::Program;
        break;
    }
    default: {
        _assert_msg_rel_(false, "unsupport comment type: {}", item_id.type());
    }
    }

    auto out = co_await c.perform(api);

    co_return out.transform([&total](const auto& out) -> qcm::oper::OperList<qcm::model::Comment> {
        total     = out.total;
        auto list = qcm::oper::CommentOper::create_list(out.comments.size());
        for (usize i = 0; i < list.size(); i++) {
            auto oper = qcm::oper::CommentOper(list.at(i));
            convert(oper, out.comments[i]);
        }
        return list;
    });
}

auto create_mix(ClientBase& cbase, QString name) -> qcm::task<Result<ItemId>> {
    auto                     c = *get_client(cbase);
    ncm::api::PlaylistCreate api;
    convert(api.input.name, name);
    co_return (co_await c.perform(api)).transform([](const auto& out) -> ItemId {
        ItemId id;
        convert(id, out.id);
        return id;
    });
}
auto delete_mix(ClientBase& cbase, std::span<const ItemId> ids) -> qcm::task<Result<bool>> {
    auto                     c = *get_client(cbase);
    ncm::api::PlaylistDelete api;
    std::ranges::copy(std::views::transform(ids,
                                            [](const ItemId& id) -> model::PlaylistId {
                                                auto out = model::PlaylistId();
                                                convert(out, id);
                                                return out;
                                            }),
                      std::back_inserter(api.input.ids));

    auto out = co_await c.perform(api);
    co_return out.transform([](const auto& out) -> bool {
        return out.code == 200;
    });
}
auto rename_mix(ClientBase& cbase, ItemId id, QString name) -> qcm::task<Result<bool>> {
    auto                         c = *get_client(cbase);
    ncm::api::PlaylistUpdateName api;
    convert(api.input.id, id);
    convert(api.input.name, name);
    co_return (co_await c.perform(api)).transform([](const auto& out) -> bool {
        return out.code == 200;
    });
}
auto manipulate_mix(ClientBase& cbase, ItemId id, qcm::enums::ManipulateMixAction act,
                    std::span<const ItemId> ids) -> qcm::task<Result<i32>> {
    auto                               c = *get_client(cbase);
    ncm::api::PlaylistManipulateTracks api;
    convert(api.input.pid, id);
    api.input.op = (ncm::params::PlaylistManipulateTracks::Oper)act;
    std::ranges::copy(std::views::transform(ids,
                                            [](const ItemId& id) -> model::SongId {
                                                auto out = model::SongId();
                                                convert(out, id);
                                                return out;
                                            }),
                      std::back_inserter(api.input.trackIds));

    co_return (co_await c.perform(api)).transform([](const auto& out) -> i32 {
        return out.code == 200 ? 1 : 0;
    });
}

} // namespace ncm::impl

namespace ncm
{

auto qml::create_client() -> qcm::Client {
    auto c = ncm::Client(
        qcm::Global::instance()->session(),
        qcm::Global::instance()->pool_executor(),
        ncm::api::device_id_from_uuid(qcm::Global::instance()->uuid().toString().toStdString()));

    auto api      = make_rc<qcm::Global::Client::Api>();
    auto instance = make_rc<ncm::impl::Client>(c);

    api->provider        = ncm::qml::provider;
    api->server_url      = ncm::impl::server_url;
    api->make_request    = ncm::impl::make_request;
    api->play_state      = ncm::impl::play_state;
    api->router          = ncm::impl::router;
    api->logout          = ncm::impl::logout;
    api->session_check   = ncm::impl::session_check;
    api->collect         = ncm::impl::collect;
    api->media_url       = ncm::impl::media_url;
    api->sync_collection = ncm::impl::sync_collection;
    api->sync_items      = ncm::impl::sync_items;
    api->sync_list       = ncm::impl::sync_list;
    api->save            = ncm::impl::save;
    api->load            = ncm::impl::load;
    api->comments        = ncm::impl::comments;
    api->create_mix      = ncm::impl::create_mix;
    api->delete_mix      = ncm::impl::delete_mix;
    api->rename_mix      = ncm::impl::rename_mix;
    api->manipulate_mix  = ncm::impl::manipulate_mix;

    return { .api = api, .instance = instance };
}

auto qml::to_ncm_client(const qcm::Client& c) -> std::optional<ncm::Client> {
    if (c.api->provider == ncm::provider) {
        return *impl::get_client(*c.instance);
    } else
        return std::nullopt;
}

auto qml::get_ncm_client() -> std::optional<ncm::Client> {
    return qcm::get_client().and_then(to_ncm_client);
}

auto qml::check(std::optional<ncm::Client> opt, const std::source_location loc) -> bool {
    if (! opt) {
        qcm::log::log(qcm::LogLevel::ERROR, loc, "client not valid");
        return false;
    }
    return true;
}

auto qml::uniq(const QUrl& url, const QVariant& info) -> QString {
    auto    size = info.value<QSize>();
    QString size_query;
    if (size.isValid()) {
        size = get_down_size(size);
    }
    return QStringLiteral("%1&param=%2y%3")
        .arg(url.toString())
        .arg(size.width())
        .arg(size.height());
}

} // namespace ncm
