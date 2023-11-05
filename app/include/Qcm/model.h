#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QDateTime>

#include "core/core.h"
#include "core/vec_helper.h"

#include "ncm/model.h"
#include "Qcm/type.h"

#define GADGET_PROPERTY(_type_, _prop_, _var_, _val_) \
    Q_PROPERTY(_type_ _prop_ MEMBER _var_)     \
    _type_ _var_ {_val_};

#define GADGET_PROPERTY_DEF(_type_, _prop_, _var_) GADGET_PROPERTY(_type_, _prop_, _var_,)

#define GATGET_LIST_PROPERTY(_type_, _prop_, _var_)                                   \
    Q_PROPERTY(QVariantList _prop_ READ get_##_prop_ WRITE set_##_prop_)              \
    QVariantList get_##_prop_() const { return QVariant::fromValue(_var_).toList(); } \
    void         set_##_prop_(const QVariantList& in) {                               \
        _var_.clear();                                                        \
        for (auto& el : in) {                                                 \
            _var_ << el.value<_type_>();                                      \
        }                                                                     \
    }                                                                                 \
    QList<_type_> _var_;

#define READ_PROPERTY(_type_, _prop_, _var_, _sig_)    \
    Q_PROPERTY(_type_ _prop_ READ _prop_ NOTIFY _sig_) \
    const auto& _prop_() const { return _var_; }       \
    _type_      _var_;

#define FORWARD_PROPERTY(_type_, _prop_, _input_)                                     \
public:                                                                               \
    Q_PROPERTY(_type_ _prop_ READ _prop_ WRITE set_##_prop_ NOTIFY changed_##_prop_)  \
    _type_ _prop_() const { return convert_from<_type_>(this->api().input._input_); } \
    void   set_##_prop_(_type_ v) {                                                   \
        auto& cur = this->api().input._input_;                                      \
        auto  v_  = convert_from<std::decay_t<decltype(cur)>>(v);                   \
        if (cur != v_) {                                                            \
            cur = v_;                                                               \
            this->mark_dirty();                                                     \
            emit changed_##_prop_();                                                \
            this->reload_if_needed();                                               \
        }                                                                           \
    }                                                                                 \
Q_SIGNALS:                                                                            \
    void changed_##_prop_();

#define FORWARD_PROPERTY_DECLARE(_type_, _prop_, _input_)                            \
public:                                                                              \
    Q_PROPERTY(_type_ _prop_ READ _prop_ WRITE set_##_prop_ NOTIFY changed_##_prop_) \
    _type_ _prop_() const;                                                           \
    void   set_##_prop_(_type_ v);                                                   \
Q_SIGNALS:                                                                           \
    void changed_##_prop_();

#define FORWARD_PROPERTY_IMPL(_class_, _type_, _prop_, _input_)   \
    inline _type_ _class_::_prop_() const {                       \
        return convert_from<_type_>(this->api().input._input_);   \
    }                                                             \
    inline void _class_::set_##_prop_(_type_ v) {                 \
        auto& cur = this->api().input._input_;                    \
        auto  v_  = convert_from<std::decay_t<decltype(cur)>>(v); \
        if (cur != v_) {                                          \
            cur = v_;                                             \
            this->mark_dirty();                                   \
            emit changed_##_prop_();                              \
            this->reload_if_needed();                             \
        }                                                         \
    }

namespace qcm
{

namespace model
{

struct ItemIdType : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
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

DECLARE_CONVERT(QDateTime, ncm::model::Time)
DECLARE_CONVERT(qcm::model::Artist, ncm::model::Artist)
DECLARE_CONVERT(qcm::model::Artist, ncm::model::Song::Ar)
DECLARE_CONVERT(qcm::model::Album, ncm::model::Album)
DECLARE_CONVERT(qcm::model::Playlist, ncm::model::Playlist)
DECLARE_CONVERT(qcm::model::Song, ncm::model::Song)
DECLARE_CONVERT(qcm::model::User, ncm::model::User)
DECLARE_CONVERT(qcm::model::Comment, ncm::model::Comment)
DECLARE_CONVERT(qcm::model::Djradio, ncm::model::Djradio)
DECLARE_CONVERT(qcm::model::Song, ncm::model::SongB)
DECLARE_CONVERT(qcm::model::Program, ncm::model::Program)