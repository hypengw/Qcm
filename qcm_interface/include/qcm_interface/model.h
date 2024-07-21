#pragma once

#include <functional>

#include <QObject>
#include <QQmlEngine>
#include <QDateTime>

#include "core/core.h"
#include "core/vec_helper.h"

#include "qcm_interface/type.h"
#include "qcm_interface/macro.h"

#include "qcm_interface/item_id.h"
namespace qcm::model
{

class Artist {
    Q_GADGET
    QML_VALUE_TYPE(t_artist)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
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
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
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
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QDateTime, updateTime, updateTime)
    GADGET_PROPERTY_DEF(qint32, playCount, playCount)
    GADGET_PROPERTY_DEF(qint32, trackCount, trackCount)
    GADGET_PROPERTY_DEF(bool, subscribed, subscribed)
    GADGET_PROPERTY_DEF(ItemId, userId, userId)

    std::strong_ordering operator<=>(const Playlist&) const = default;
};

class Song {
    Q_GADGET
    QML_VALUE_TYPE(t_song)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
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
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)

    std::strong_ordering operator<=>(const User&) const = default;
};

class Comment {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
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
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
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
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QDateTime, duration, duration)
    GADGET_PROPERTY_DEF(QString, coverUrl, coverUrl)
    GADGET_PROPERTY_DEF(Song, song, song)
    GADGET_PROPERTY_DEF(QDateTime, createTime, createTime)
    GADGET_PROPERTY_DEF(qint32, serialNum, serialNum)

    std::strong_ordering operator<=>(const Program&) const = default;
};

} // namespace qcm::model

template<>
struct fmt::formatter<qcm::model::ItemId> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const qcm::model::ItemId& it, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(it.toUrl().toString().toStdString(), ctx);
    }
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