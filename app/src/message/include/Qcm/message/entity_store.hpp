#pragma once

#include "Qcm/message/entity.qpb.h"
#include "Qcm/message/model.qpb.h"
#include <QtCore/QHash>
#include <QtCore/QMetaProperty>
#include <QtCore/QSet>
#include <QtCore/QVariant>
#include <algorithm>
#include <array>
#include <cctype>
#include <optional>

namespace qcm::store
{

namespace wire  = qcm::msg::entity;
namespace model = qcm::msg::model;

enum class Field : unsigned
{
    Name,
    SortName,
    Type,
    Duration,
    TrackCount,
    DiscCount,
    Description,
    Company,
    Language,
    PublishTime,
    AddedAt,
    TrackNumber,
    DiscNumber,
    CanPlay,
    Popularity,
    Tags,
    AlbumCount,
    MusicCount,
    MixType,
    UpdateAt,
    Album,
    Artists,
    Favorite,
    Presence,
    Stale,
    Deleted,
    Count
};
using Fields = quint64;
constexpr Fields bit(Field field) { return Fields(1) << unsigned(field); }
constexpr Fields AllFields = (Fields(1) << unsigned(Field::Count)) - 1;

struct Key {
    int    type {};
    qint64 id {};
    bool   operator==(const Key&) const = default;
};
inline size_t qHash(const Key& key, size_t seed = 0) { return qHashMulti(seed, key.type, key.id); }
constexpr int Song = 101, Album = 51, Artist = 53, Mix = 54;
inline Key    canonical(Key key) {
    if (key.type == 52) key.type = Artist;
    return key;
}

inline std::optional<Field> field(const QByteArray& name) {
    static constexpr std::array names { "name",       "sortName",    "type",        "duration",
                                        "trackCount", "discCount",   "description", "company",
                                        "language",   "publishTime", "addedAt",     "trackNumber",
                                        "discNumber", "canPlay",     "popularity",  "tags",
                                        "albumCount", "musicCount",  "mixType",     "updateAt" };
    for (unsigned i = 0; i < names.size(); ++i)
        if (name == names[i]) return Field(i);
    return {};
}

struct Entry {
    QVariant                                                   data;
    qint64                                                     owner {};
    quint64                                                    epoch {};
    std::array<std::optional<quint64>, unsigned(Field::Count)> versions;
    Fields                                                     nulls {};
    QList<qint64>                                              artists;
    std::optional<qint64>                                      album;
    std::optional<bool>                                        favorite;
    bool                                                       stale {};
    quint64                                                    touched {};
    bool known(Field field) const { return versions[unsigned(field)].has_value(); }
};

enum class Status
{
    Missing,
    Ready,
    Stale,
    Deleted,
    Invalidated,
    Evicted
};

struct Change {
    Key    key;
    Fields fields {};
    Status status { Status::Ready };
};
struct ApplyResult {
    QString       error;
    QList<Change> changes;
    QList<Key>    stateChanges;
    explicit      operator bool() const { return error.isEmpty(); }
};

class EntityStore {
public:
    const Entry* find(Key key) const;
    template<class T>
    T value(Key key) const {
        if (auto entry = find(key)) return entry->data.value<T>();
        return {};
    }
    const QString& instance() const;
    qsizetype      size() const;
    quint64        scopeRevision() const;
    Status         status(Key key) const;
    QList<Change>  clear();
    QList<Change>  markStale();
    void           pin(Key key);
    void           unpin(Key key);
    void           setCapacity(qsizetype capacity);

    void      setByteBudget(qsizetype bytes);
    void      setTombstoneCapacity(qsizetype capacity);
    qsizetype bytes() const;

    ApplyResult apply(const wire::EntityBatch& batch);

    QList<Change> collect();

private:
    static qsizetype          entryBytes(const Entry& entry);
    void                      rememberAbsence(Key key, Status status);
    void                      rebuildDependencies();
    quint64                   m_scopeRevision {};
    QHash<Key, Entry>         m_entries;
    QHash<Key, QList<Change>> m_dependents;
    QHash<Key, qsizetype>     m_pins;
    QHash<Key, quint64>       m_tombstones;
    QHash<Key, Status>        m_absent;
    quint64                   m_floor {};
    QHash<qint64, quint64>    m_epochs;
    QString                   m_instance;
    quint64                   m_clock {};
    qsizetype                 m_capacity { 10000 };
    qsizetype                 m_byteBudget { 64 * 1024 * 1024 };
    qsizetype                 m_tombstoneCapacity { 8192 };
};

} // namespace qcm::store
