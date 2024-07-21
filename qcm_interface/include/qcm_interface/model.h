#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QDateTime>

#include "core/core.h"
#include "core/vec_helper.h"

#include "qcm_interface/type.h"
#include "qcm_interface/macro.h"

namespace qcm
{

namespace model
{

struct ItemIdType : public QObject {
    Q_OBJECT
public:
    enum Type
    {
        Unknown,
        Song,
        Album,
        Artist,
        Playlist,
        User,
        Comment,
        Djradio,
        Program
    };
    Q_ENUM(Type)
};

struct ItemId {
    Q_GADGET
public:
    using Type = ItemIdType::Type;

    Q_PROPERTY(ItemId::Type type READ type)
    Q_PROPERTY(QString sid READ id_)
    Q_PROPERTY(ItemIdType* objectType READ objectType)

    Q_INVOKABLE bool valid() const {
        auto& id = id_();
        return ! id.isEmpty() && id != "0";
    }

    ItemId(QString id = ""): id(id) {}
    virtual ~ItemId() = default;

    const QString&       id_() const { return id; }
    virtual ItemId::Type type() const { return Type::Unknown; }
    ItemIdType*          objectType() const {
        static ItemIdType t;
        return &t;
    }

    std::strong_ordering operator<=>(const ItemId&) const = default;

    QString id;
};

template<ItemId::Type TType>
struct ItemIdBase : public ItemId {
    using ItemId::ItemId;
    virtual ~ItemIdBase() = default;

    Type                 type() const override { return TType; }
    std::strong_ordering operator<=>(const ItemIdBase<TType>&) const = default;
};

template<typename T>
concept ItemIdCP = requires(T t) {
    { t.id } -> std::convertible_to<QString>;
    { t.type() } -> std::same_as<ItemId::Type>;
};

struct SongId : public ItemIdBase<ItemId::Type::Song> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::Song>::ItemIdBase;
    virtual ~SongId() = default;

    Q_INVOKABLE QUrl url() const {
        return QUrl(convert_from<QString>(fmt::format("https://music.163.com/song?id={}", id)));
    }
};

struct AlbumId : public ItemIdBase<ItemId::Type::Album> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::Album>::ItemIdBase;
    virtual ~AlbumId() = default;
};

struct ArtistId : public ItemIdBase<ItemId::Type::Artist> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::Artist>::ItemIdBase;
    virtual ~ArtistId() = default;
};
struct PlaylistId : public ItemIdBase<ItemId::Type::Playlist> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::Playlist>::ItemIdBase;
    virtual ~PlaylistId() = default;
};

struct UserId : public ItemIdBase<ItemId::Type::User> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::User>::ItemIdBase;
    virtual ~UserId() = default;
};

struct CommentId : public ItemIdBase<ItemId::Type::Comment> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::Comment>::ItemIdBase;
    virtual ~CommentId() = default;
};
struct DjradioId : public ItemIdBase<ItemId::Type::Djradio> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::Djradio>::ItemIdBase;
    virtual ~DjradioId() = default;
};
struct ProgramId : public ItemIdBase<ItemId::Type::Program> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Type::Program>::ItemIdBase;
    virtual ~ProgramId() = default;
};

class Artist {
    Q_GADGET
    QML_VALUE_TYPE(t_artist)
public:
    GADGET_PROPERTY_DEF(ArtistId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(QString, briefDesc, briefDesc)
    GADGET_PROPERTY_DEF(qint32, albumSize, albumSize)
    GADGET_PROPERTY_DEF(qint32, musicSize, musicSize)
    GADGET_PROPERTY_DEF(std::vector<QString>, alias, alias)
    GADGET_PROPERTY_DEF(bool, followed, followed)

    std::strong_ordering operator<=>(const Artist&) const = default;
};

class Album {
    Q_GADGET
    QML_VALUE_TYPE(t_album)
public:
    GADGET_PROPERTY_DEF(AlbumId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(QDateTime, publishTime, publishTime)
    GADGET_PROPERTY_DEF(int, trackCount, trackCount)
    GADGET_PROPERTY_DEF(bool, subscribed, subscribed)
    GATGET_LIST_PROPERTY(Artist, artists, artists)

    std::strong_ordering operator<=>(const Album&) const = default;
};

class Playlist {
    Q_GADGET
    QML_VALUE_TYPE(t_playlist)
public:
    GADGET_PROPERTY_DEF(PlaylistId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QDateTime, updateTime, updateTime)
    GADGET_PROPERTY_DEF(qint32, playCount, playCount)
    GADGET_PROPERTY_DEF(qint32, trackCount, trackCount)
    GADGET_PROPERTY_DEF(bool, subscribed, subscribed)
    GADGET_PROPERTY_DEF(UserId, userId, userId)

    std::strong_ordering operator<=>(const Playlist&) const = default;
};

class Song {
    Q_GADGET
    QML_VALUE_TYPE(t_song)
public:
    GADGET_PROPERTY_DEF(SongId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(Album, album, album)
    GADGET_PROPERTY_DEF(QDateTime, duration, duration)
    GADGET_PROPERTY_DEF(bool, canPlay, canPlay)
    GADGET_PROPERTY_DEF(QString, coverUrl, coverUrl)
    GADGET_PROPERTY_DEF(QList<QString>, tags, tags)

    GADGET_PROPERTY_DEF(QVariant, source, source)

    GATGET_LIST_PROPERTY(Artist, artists, artists)

    std::strong_ordering operator<=>(const Song&) const = default;
};

class User {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(UserId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)

    std::strong_ordering operator<=>(const User&) const = default;
};

class Comment {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(CommentId, itemId, id)
    GADGET_PROPERTY_DEF(User, user, user)
    GADGET_PROPERTY_DEF(QString, content, content)
    GADGET_PROPERTY_DEF(QDateTime, time, time)
    GADGET_PROPERTY_DEF(bool, liked, liked)

    std::strong_ordering operator<=>(const Comment&) const = default;
};

class Djradio {
    Q_GADGET
    QML_VALUE_TYPE(t_djradio)
public:
    GADGET_PROPERTY_DEF(DjradioId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(std::vector<Artist>, artists, artists)
    GADGET_PROPERTY_DEF(qint32, programCount, programCount)

    std::strong_ordering operator<=>(const Djradio&) const = default;
};

class Program {
    Q_GADGET
    QML_VALUE_TYPE(t_program)
public:
    GADGET_PROPERTY_DEF(ProgramId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QDateTime, duration, duration)
    GADGET_PROPERTY_DEF(QString, coverUrl, coverUrl)
    GADGET_PROPERTY_DEF(Song, song, song)
    GADGET_PROPERTY_DEF(QDateTime, createTime, createTime)
    GADGET_PROPERTY_DEF(qint32, serialNum, serialNum)

    std::strong_ordering operator<=>(const Program&) const = default;
};

} // namespace model
} // namespace qcm

template<qcm::model::ItemIdCP I>
struct fmt::formatter<I> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const I& it, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(it.id.toStdString(), ctx);
    }
};

template<typename T, typename F>
    requires qcm::model::ItemIdCP<T> && (std::same_as<F, i64> || std::same_as<F, std::string>)
struct Convert<T, F> {
    Convert(T& out, const F& in) { out = convert_from<QString>(in); }
};

namespace qcm::model
{

namespace details
{
template<typename T>
class ModelBase : public T {};

template<>
class ModelBase<void> {};
} // namespace details

constexpr auto MF_COPY { 1 };

template<typename T, typename TBase>
class Model : public details::ModelBase<TBase> {
    using Base = details::ModelBase<TBase>;

protected:
    Model();
    ~Model();

    Model(const Model&)            = delete;
    Model& operator=(const Model&) = delete;

    class Private;
    inline auto d_func() -> Private* { return m_ptr.get(); }
    inline auto d_func() const -> const Private* { return m_ptr.get(); };

private:
    up<Private> m_ptr;
};

template<typename T, typename TBase>
    requires std::same_as<void, TBase> || std::copy_constructible<TBase>
class Model<T, TBase> : public details::ModelBase<TBase> {
    using Base = details::ModelBase<TBase>;

protected:
    Model();
    ~Model();

    Model(const Model&);
    Model& operator=(const Model&);

    class Private;
    inline auto d_func() -> Private* { return m_ptr.get(); }
    inline auto d_func() const -> const Private* { return m_ptr.get(); };

private:
    up<Private> m_ptr;
};

} // namespace qcm::model