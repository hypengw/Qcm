#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QDateTime>

#include "core/core.h"
#include "core/vec_helper.h"

#include "ncm/model.h"
#include "Qcm/type.h"

#define GATGET_PROPERTY(_type_, _prop_, _var_) \
    Q_PROPERTY(_type_ _prop_ MEMBER _var_)     \
    _type_ _var_;

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
public:
    GATGET_PROPERTY(ArtistId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(QString, briefDesc, briefDesc)
    GATGET_PROPERTY(qint32, albumSize, albumSize)
    GATGET_PROPERTY(qint32, musicSize, musicSize)
    GATGET_PROPERTY(std::vector<QString>, alias, alias)

    std::strong_ordering operator<=>(const Artist&) const = default;
};

class Album {
    Q_GADGET
public:
    GATGET_PROPERTY(AlbumId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(QDateTime, publishTime, publishTime)
    GATGET_PROPERTY(int, trackCount, trackCount)
    GATGET_LIST_PROPERTY(Artist, artists, artists)

    std::strong_ordering operator<=>(const Album&) const = default;
};

class Playlist {
    Q_GADGET
public:
    GATGET_PROPERTY(PlaylistId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(QString, description, description)
    GATGET_PROPERTY(QDateTime, updateTime, updateTime)
    GATGET_PROPERTY(qint32, playCount, playCount)
    GATGET_PROPERTY(qint32, trackCount, trackCount)

    std::strong_ordering operator<=>(const Playlist&) const = default;
};

class Song {
    Q_GADGET
    QML_VALUE_TYPE(t_song)
public:
    GATGET_PROPERTY(SongId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(Album, album, album)
    GATGET_PROPERTY(QDateTime, duration, duration)
    GATGET_PROPERTY(bool, canPlay, canPlay)
    GATGET_PROPERTY(QString, coverUrl, coverUrl)
    GATGET_PROPERTY(QList<QString>, tags, tags)

    GATGET_LIST_PROPERTY(Artist, artists, artists)

    std::strong_ordering operator<=>(const Song&) const = default;
};

class User {
    Q_GADGET
public:
    GATGET_PROPERTY(UserId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)

    std::strong_ordering operator<=>(const User&) const = default;
};

class Comment {
    Q_GADGET
public:
    GATGET_PROPERTY(CommentId, itemId, id)
    GATGET_PROPERTY(User, user, user)
    GATGET_PROPERTY(QString, content, content)
    GATGET_PROPERTY(bool, liked, liked)
    GATGET_PROPERTY(QDateTime, time, time)

    std::strong_ordering operator<=>(const Comment&) const = default;
};

class Djradio {
    Q_GADGET
public:
    GATGET_PROPERTY(DjradioId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(std::vector<Artist>, artists, artists)
    GATGET_PROPERTY(qint32, programCount, programCount)

    std::strong_ordering operator<=>(const Djradio&) const = default;
};

class Program {
    Q_GADGET
    QML_VALUE_TYPE(t_program)
public:
    GATGET_PROPERTY(ProgramId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QDateTime, duration, duration)
    GATGET_PROPERTY(QString, coverUrl, coverUrl)
    GATGET_PROPERTY(Song, song, song)
    GATGET_PROPERTY(QDateTime, createTime, createTime)
    GATGET_PROPERTY(qint32, serialNum, serialNum)

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