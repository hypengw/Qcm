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

#define FORWARD_PROPERTY(_type_, _prop_, _input_)                                    \
public:                                                                              \
    Q_PROPERTY(_type_ _prop_ READ _prop_ WRITE set_##_prop_ NOTIFY changed_##_prop_) \
    _type_ _prop_() const { return To<_type_>::from(this->api().input._input_); }    \
    void   set_##_prop_(_type_ v) {                                                  \
        auto& cur = this->api().input._input_;                                     \
        auto  v_  = To<std::decay_t<decltype(cur)>>::from(v);                      \
        if (cur != v_) {                                                           \
            cur = v_;                                                              \
            this->mark_dirty();                                                    \
            emit changed_##_prop_();                                               \
            this->reload_if_needed();                                              \
        }                                                                          \
    }                                                                                \
Q_SIGNALS:                                                                           \
    void changed_##_prop_();

#define FORWARD_PROPERTY_DECLARE(_type_, _prop_, _input_)                            \
public:                                                                              \
    Q_PROPERTY(_type_ _prop_ READ _prop_ WRITE set_##_prop_ NOTIFY changed_##_prop_) \
    _type_ _prop_() const;                                                           \
    void   set_##_prop_(_type_ v);                                                   \
Q_SIGNALS:                                                                           \
    void changed_##_prop_();

#define FORWARD_PROPERTY_IMPL(_class_, _type_, _prop_, _input_)                                   \
    inline _type_ _class_::_prop_() const { return To<_type_>::from(this->api().input._input_); } \
    inline void   _class_::set_##_prop_(_type_ v) {                                               \
        auto& cur = this->api().input._input_;                                                  \
        auto  v_  = To<std::decay_t<decltype(cur)>>::from(v);                                   \
        if (cur != v_) {                                                                        \
            cur = v_;                                                                           \
            this->mark_dirty();                                                                 \
            emit changed_##_prop_();                                                            \
            this->reload_if_needed();                                                           \
        }                                                                                       \
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
        return QUrl(To<QString>::from(fmt::format("https://music.163.com/song?id={}", id)));
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
    GATGET_PROPERTY(QList<QString>, tags, tags)

    GATGET_LIST_PROPERTY(Artist, artists, artists)

    std::strong_ordering operator<=>(const Song&) const = default;
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

template<typename T>
    requires qcm::model::ItemIdCP<T>
struct To<T> {
    static auto from(i64 s) { return T { QString::fromStdString(To<std::string>::from(s)) }; }
    static auto from(const std::string& s) { return T { QString::fromStdString(s) }; }
};

template<>
template<>
struct To<QDateTime>::From<ncm::model::Time> {
    static QDateTime from(const ncm::model::Time& t);
};

template<>
struct To<qcm::model::Artist> {
    static qcm::model::Artist from(const ncm::model::Artist& in);
    static qcm::model::Artist from(const ncm::model::Song::Ar& in);
};

template<>
struct To<qcm::model::Album> {
    static qcm::model::Album from(const ncm::model::Album& in);
};

template<>
template<>
struct To<qcm::model::Playlist>::From<ncm::model::Playlist> {
    static out_type from(const ncm::model::Playlist&);
};

template<>
struct To<qcm::model::Song> {
    static qcm::model::Song from(const ncm::model::Song& in);
};
