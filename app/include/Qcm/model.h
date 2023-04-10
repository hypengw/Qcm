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

#define CONVERT_PROPERTY(_out_, _in_) _out_ = To<std::decay_t<decltype(_out_)>>::from(_in_)

namespace qcm
{
namespace model
{

struct ItemId : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    enum Type
    {
        Song,
        Album,
        Artist,
        Playlist,
        User,
    };
    Q_ENUM(Type)
};

struct ItemIdGad {
    Q_GADGET
public:
    Q_PROPERTY(ItemId::Type type READ type)
    Q_PROPERTY(QString sid READ id_)

    Q_INVOKABLE bool valid() const {
        auto& id = id_();
        return ! id.isEmpty() && id != "0";
    }

    virtual ItemId::Type   type() const = 0;
    virtual const QString& id_() const  = 0;

    std::strong_ordering operator<=>(const ItemIdGad&) const = default;
};

template<ItemId::Type Type>
struct ItemIdBase : public ItemIdGad {
    ItemIdBase(QString id = ""): id(id) {}
    virtual ~ItemIdBase() = default;

    ItemId::Type   type() const override { return Type; }
    const QString& id_() const override { return id; }

    QString id;

    std::strong_ordering operator<=>(const ItemIdBase<Type>&) const = default;
};

template<typename T>
concept ItemIdCP = requires(T t) {
                       { t.id } -> std::convertible_to<QString>;
                       { t.type() } -> std::same_as<ItemId::Type>;
                   };

struct SongId : public ItemIdBase<ItemId::Song> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Song>::ItemIdBase;
    virtual ~SongId() = default;

    Q_INVOKABLE QUrl url() const {
        return QUrl(To<QString>::from(fmt::format("https://music.163.com/song?id={}", id)));
    }
};

struct AlbumId : public ItemIdBase<ItemId::Album> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Album>::ItemIdBase;
    virtual ~AlbumId() = default;
};

struct ArtistId : public ItemIdBase<ItemId::Artist> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Artist>::ItemIdBase;
    virtual ~ArtistId() = default;
};
struct PlaylistId : public ItemIdBase<ItemId::Playlist> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::Playlist>::ItemIdBase;
    virtual ~PlaylistId() = default;
};

struct UserId : public ItemIdBase<ItemId::User> {
    Q_GADGET
public:
    using ItemIdBase<ItemId::User>::ItemIdBase;
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
    GATGET_PROPERTY(std::vector<Artist>, artists, artists)

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
    GATGET_PROPERTY(std::vector<Artist>, artists, artists)
    GATGET_PROPERTY(bool, canPlay, canPlay)
    GATGET_PROPERTY(std::vector<QString>, tags, tags)

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
struct To<QDateTime> {
    template<typename T>
    struct From;

    template<typename T>
    static QDateTime from(const T& t) {
        return From<T>::from(t);
    }
};
template<>
struct To<QDateTime>::From<ncm::model::Time> {
    static auto from(const ncm::model::Time& t) {
        return QDateTime::fromMSecsSinceEpoch(t.milliseconds);
    }
};

template<>
struct To<qcm::model::Artist> {
    static qcm::model::Artist from(const ncm::model::Artist& in) {
        qcm::model::Artist o;
        CONVERT_PROPERTY(o.id, in.id);
        CONVERT_PROPERTY(o.name, in.name);
        CONVERT_PROPERTY(o.picUrl, in.picUrl);
        CONVERT_PROPERTY(o.briefDesc, in.briefDesc.value_or(""));
        CONVERT_PROPERTY(o.musicSize, in.musicSize);
        CONVERT_PROPERTY(o.albumSize, in.albumSize);
        return o;
    }
    static qcm::model::Artist from(const ncm::model::Song::Ar& in) {
        qcm::model::Artist o;
        CONVERT_PROPERTY(o.id, in.id);
        CONVERT_PROPERTY(o.name, in.name);
        CONVERT_PROPERTY(o.alias, in.alia);
        return o;
    }
};

template<>
struct To<qcm::model::Album> {
    static qcm::model::Album from(const ncm::model::Album& in) {
        qcm::model::Album o;
        CONVERT_PROPERTY(o.id, in.id);
        CONVERT_PROPERTY(o.name, in.name);
        CONVERT_PROPERTY(o.picUrl, in.picUrl);
        CONVERT_PROPERTY(o.artists, in.artists);
        CONVERT_PROPERTY(o.publishTime, in.publishTime);
        return o;
    }
};

template<>
struct To<qcm::model::Playlist> {
    template<typename T>
    static auto from(const T&);
};
template<>
inline auto To<qcm::model::Playlist>::from(const ncm::model::Playlist& in) {
    qcm::model::Playlist o;
    CONVERT_PROPERTY(o.id, in.id);
    CONVERT_PROPERTY(o.name, in.name);
    CONVERT_PROPERTY(o.picUrl, in.coverImgUrl);
    CONVERT_PROPERTY(o.description, in.description.value_or(""));
    CONVERT_PROPERTY(o.updateTime, in.updateTime);
    CONVERT_PROPERTY(o.playCount, in.playCount);
    return o;
}

template<>
struct To<qcm::model::Song> {
    static qcm::model::Song from(const ncm::model::Song& in) {
        qcm::model::Song o;
        CONVERT_PROPERTY(o.id, in.id);
        CONVERT_PROPERTY(o.name, in.name);
        CONVERT_PROPERTY(o.album.id, in.al.id);
        CONVERT_PROPERTY(o.album.name, in.al.name);
        CONVERT_PROPERTY(o.album.picUrl, in.al.picUrl);
        CONVERT_PROPERTY(o.duration, in.dt);
        CONVERT_PROPERTY(o.artists, in.ar);
        CONVERT_PROPERTY(o.canPlay, (! in.privilege || in.privilege.value().st >= 0));

        if (in.privilege) {
            QString tag;
            auto    fee = in.privilege.value().fee;
            switch (fee) {
                using enum ncm::model::SongFee;
            case Vip: tag = "vip"; break;
            case OnlyOnlineWithPaid:
            case OnlyDownloadWithPaid: tag = "pay"; break;
            case DigitalAlbum: tag = "dg"; break;
            case Free:
            case Free128k: break;
            default: WARN_LOG("unknown fee: {}, {}", (i64)fee, in.name);
            }
            if (! tag.isEmpty()) o.tags.push_back(tag);
        }
        return o;
    }
};
