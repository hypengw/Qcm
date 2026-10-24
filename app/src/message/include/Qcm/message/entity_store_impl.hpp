#pragma once
#include "entity_store.hpp"

namespace qcm::store
{

static void trimAbsences(QHash<Key, Status>& absent, const QHash<Key, qsizetype>& pins,
                         qsizetype capacity) {
    const auto limit = std::max<qsizetype>(64, capacity) + pins.size();
    for (auto it = absent.begin(); it != absent.end() && absent.size() > limit;)
        if (! pins.contains(it.key()))
            it = absent.erase(it);
        else
            ++it;
}

const Entry* EntityStore::find(Key key) const {
    auto it = m_entries.constFind(canonical(key));
    return it == m_entries.cend() ? nullptr : &it.value();
}

const QString& EntityStore::instance() const { return m_instance; }

qsizetype EntityStore::size() const { return m_entries.size(); }

quint64 EntityStore::scopeRevision() const { return m_scopeRevision; }

Status EntityStore::status(Key key) const {
    if (auto entry = find(key)) return entry->stale ? Status::Stale : Status::Ready;
    auto it = m_absent.constFind(canonical(key));
    return it == m_absent.cend() ? Status::Missing : it.value();
}

QList<Change> EntityStore::clear() {
    QList<Change> changes;
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it)
        changes.push_back({ it.key(), AllFields, Status::Invalidated });
    ++m_scopeRevision;
    m_entries.clear();
    m_dependents.clear();
    m_epochs.clear();
    m_tombstones.clear();
    m_absent.clear();
    for (const auto& change : changes) rememberAbsence(change.key, Status::Invalidated);
    m_floor = 0;
    m_instance.clear();
    return changes;
}

QList<Change> EntityStore::markStale() {
    QList<Change> changes;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        if (! it->stale) {
            it->stale = true;
            changes.push_back({ it.key(), bit(Field::Stale), Status::Stale });
        }
    return changes;
}

void EntityStore::pin(Key key) { ++m_pins[canonical(key)]; }

void EntityStore::unpin(Key key) {
    auto it = m_pins.find(canonical(key));
    if (it != m_pins.end() && ! --it.value()) m_pins.erase(it);
}

void EntityStore::setCapacity(qsizetype capacity) { m_capacity = std::max<qsizetype>(0, capacity); }

void EntityStore::setByteBudget(qsizetype bytes) { m_byteBudget = std::max<qsizetype>(0, bytes); }

void EntityStore::setTombstoneCapacity(qsizetype capacity) {
    m_tombstoneCapacity = std::max<qsizetype>(0, capacity);
}

qsizetype EntityStore::bytes() const {
    qsizetype bytes = m_tombstones.size() * (sizeof(Key) + sizeof(quint64) + 32) +
                      m_absent.size() * (sizeof(Key) + sizeof(Status) + 32) +
                      m_pins.size() * (sizeof(Key) + sizeof(qsizetype) + 32);
    for (const auto& entry : m_entries) bytes += entryBytes(entry);
    return bytes;
}

ApplyResult EntityStore::apply(const wire::EntityBatch& batch) {
    ApplyResult result;
    if (batch.protocol() != 1 || batch.instance().isEmpty()) {
        result.error = QStringLiteral("Unsupported entity protocol");
        return result;
    }
    if (! m_instance.isEmpty() && m_instance != batch.instance()) {
        result.error = QStringLiteral("Entity backend instance changed");
        return result;
    }
    const auto count = batch.songs().size() + batch.albums().size() + batch.artists().size() +
                       batch.mixes().size() + batch.removals().size();
    if (count > 200000 || batch.invalidations().size() > 10000) {
        result.error = QStringLiteral("Entity batch budget exceeded");
        return result;
    }
    QHash<Key, Entry>  staged;
    QSet<Key>          removed;
    auto               epochs     = m_epochs;
    auto               tombstones = m_tombstones;
    QHash<Key, Fields> changed;
    QHash<Key, Status> statuses;
    auto               fail = [&](QString message) {
        if (result.error.isEmpty()) result.error = std::move(message);
    };
    for (const auto& reset : batch.invalidations()) {
        if (reset.revision() == 0 || reset.owner() < 0) {
            fail(QStringLiteral("Invalid scope stamp"));
            break;
        }
        auto previous = epochs.value(reset.owner(), 0);
        if (reset.epoch() > previous) {
            epochs.insert(reset.owner(), reset.epoch());
            for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it)
                if (it->owner == reset.owner() && it->epoch < reset.epoch()) {
                    removed.insert(it.key());
                    changed[it.key()]  = AllFields;
                    statuses[it.key()] = Status::Invalidated;
                }
        }
    }
    auto prepare = [&](Key key, const wire::ReadStamp& stamp, QVariant data) -> Entry* {
        if (key.id <= 0 || stamp.revision() == 0 || stamp.owner() < 0) {
            fail(QStringLiteral("Invalid entity identity or stamp"));
            return nullptr;
        }
        if (stamp.revision() < m_floor) return nullptr;
        const auto epoch = epochs.value(stamp.owner(), 0);
        if (stamp.epoch() < epoch) return nullptr;
        if (stamp.epoch() > epoch) {
            fail(QStringLiteral("Unannounced owner epoch"));
            return nullptr;
        }
        if (auto dead = tombstones.constFind(key);
            dead != tombstones.cend() && dead.value() >= stamp.revision())
            return nullptr;
        if (! staged.contains(key)) {
            if (auto existing = find(key); existing && ! removed.contains(key)) {
                if (existing->owner != stamp.owner()) {
                    fail(QStringLiteral("Entity owner changed"));
                    return nullptr;
                }
                staged.insert(key, *existing);
            } else {
                Entry entry;
                entry.data  = std::move(data);
                entry.owner = stamp.owner();
                entry.epoch = stamp.epoch();
                staged.insert(key, std::move(entry));
                changed[key] |= bit(Field::Presence);
            }
        }
        removed.remove(key);
        auto& entry = staged[key];
        if (entry.stale) {
            entry.stale = false;
            changed[key] |= bit(Field::Stale);
        }
        auto& presence = entry.versions[unsigned(Field::Presence)];
        presence       = std::max(presence.value_or(0), quint64(stamp.revision()));
        statuses[key]  = Status::Ready;
        return &entry;
    };
    auto assign = [&](Key             key,
                      Entry&          entry,
                      Field           slot,
                      quint64         revision,
                      const QVariant& incoming,
                      bool            isNull,
                      const QVariant& previous,
                      auto            write) {
        auto& seen = entry.versions[unsigned(slot)];
        if (seen && *seen > revision) return;
        bool different = ! seen || previous != incoming || bool(entry.nulls & bit(slot)) != isNull;
        if (seen && *seen == revision && different) {
            fail(QStringLiteral("Conflicting entity field revision"));
            return;
        }
        if (different) {
            write();
            changed[key] |= bit(slot);
        }
        seen        = revision;
        entry.nulls = isNull ? entry.nulls | bit(slot) : entry.nulls & ~bit(slot);
    };
    auto metadata = [&]<class Data>(Key key, Entry& entry, const auto& patch, quint64 revision) {
        const auto* meta   = &std::remove_cvref_t<decltype(patch)>::staticMetaObject;
        const auto* target = &Data::staticMetaObject;
        auto        data   = entry.data.value<Data>();
        for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
            const auto property = meta->property(i);
            QByteArray name(property.name());
            if (name.startsWith("has") || name.endsWith("_p")) continue;
            const auto flags = property.metaType().flags();
            if (flags.testFlag(QMetaType::IsGadget) || flags.testFlag(QMetaType::PointerToGadget))
                continue;
            auto slot = field(name);
            if (! slot) continue;
            QByteArray has = "has" + name;
            has[3]         = char(std::toupper(static_cast<unsigned char>(has[3])));
            int presence   = meta->indexOfProperty(has);
            if (presence < 0 || ! meta->property(presence).readOnGadget(&patch).toBool()) continue;
            auto destination = target->property(target->indexOfProperty(name));
            if (! destination.isValid()) {
                fail(QStringLiteral("Unknown entity field"));
                continue;
            }
            QVariant incoming = property.readOnGadget(&patch);
            auto     previous = destination.readOnGadget(&data);
            assign(key, entry, *slot, revision, incoming, false, previous, [&] {
                if (! destination.writeOnGadget(&data, incoming))
                    fail(QStringLiteral("Invalid entity field type"));
            });
        }
        auto text =
            [&](Field slot, const wire::TextValue& wrapped, const QString& previous, auto write) {
                QString incoming = wrapped.hasValue() ? wrapped.value() : QString();
                assign(key, entry, slot, revision, incoming, ! wrapped.hasValue(), previous, [&] {
                    write(incoming);
                });
            };
        auto time =
            [&](Field slot, const wire::TimeValue& wrapped, const auto& previous, auto write) {
                google::protobuf::Timestamp incoming =
                    wrapped.hasValue() ? wrapped.value() : google::protobuf::Timestamp();
                assign(key,
                       entry,
                       slot,
                       revision,
                       QVariant::fromValue(incoming),
                       ! wrapped.hasValue(),
                       QVariant::fromValue(previous),
                       [&] {
                           write(incoming);
                       });
            };
        if (patch.hasSortName())
            text(Field::SortName, patch.sortName(), data.sortName(), [&](auto value) {
                data.setSortName(value);
            });
        if constexpr (requires { patch.company(); }) {
            if (patch.hasDescription())
                text(Field::Description, patch.description(), data.description(), [&](auto value) {
                    data.setDescription(value);
                });
            if (patch.hasCompany())
                text(Field::Company, patch.company(), data.company(), [&](auto value) {
                    data.setCompany(value);
                });
            if (patch.hasLanguage())
                text(Field::Language, patch.language(), data.language(), [&](auto value) {
                    data.setLanguage(value);
                });
        }
        if constexpr (requires { patch.publishTime(); }) {
            if (patch.hasPublishTime())
                time(Field::PublishTime, patch.publishTime(), data.publishTime(), [&](auto value) {
                    data.setPublishTime(value);
                });
        }
        if constexpr (requires { patch.addedAt(); }) {
            if (patch.hasAddedAt())
                time(Field::AddedAt, patch.addedAt(), data.addedAt(), [&](auto value) {
                    data.setAddedAt(value);
                });
        }
        if constexpr (requires { patch.updateAt(); }) {
            if (patch.hasUpdateAt())
                time(Field::UpdateAt, patch.updateAt(), data.updateAt(), [&](auto value) {
                    data.setUpdateAt(value);
                });
        }
        if constexpr (requires { patch.tags(); }) {
            if (patch.hasTags())
                assign(key,
                       entry,
                       Field::Tags,
                       revision,
                       patch.tags().values(),
                       false,
                       data.tags(),
                       [&] {
                           data.setTags(patch.tags().values());
                       });
        }
        entry.data = QVariant::fromValue(data);
    };
    auto state = [&](Key key, Entry& entry, const wire::ItemState& value, quint64 revision) {
        auto previous = entry.favorite;
        if (value.hasFavorite())
            assign(key,
                   entry,
                   Field::Favorite,
                   revision,
                   bool(value.favorite()),
                   false,
                   entry.favorite ? QVariant(*entry.favorite) : QVariant(),
                   [&] {
                       entry.favorite = value.favorite();
                   });
        if (previous && entry.favorite != previous) result.stateChanges.push_back(key);
    };
    auto artists = [&](Key key, Entry& entry, const wire::EntityIds& value, quint64 revision) {
        if (value.ids().size() > 10000) {
            fail(QStringLiteral("Artist relation budget exceeded"));
            return;
        }
        QList<qint64> ids;
        QSet<qint64>  seen;
        for (auto id : value.ids()) {
            if (id <= 0 || seen.contains(id)) {
                fail(QStringLiteral("Invalid artist relation"));
                return;
            }
            ids.push_back(id);
            seen.insert(id);
        }
        assign(key,
               entry,
               Field::Artists,
               revision,
               QVariant::fromValue(ids),
               false,
               QVariant::fromValue(entry.artists),
               [&] {
                   entry.artists = std::move(ids);
               });
    };
    for (const auto& update : batch.songs()) {
        model::Song data;
        data.setId_proto(update.id_proto());
        Key  key { Song, update.id_proto() };
        auto entry = prepare(key, update.stamp(), QVariant::fromValue(data));
        if (! entry) continue;
        const auto        revision = update.stamp().revision();
        metadata.template operator()<model::Song>(key, *entry, update.fields(), revision);
        state(key, *entry, update.state(), revision);
        if (update.relations().hasArtists())
            artists(key, *entry, update.relations().artists(), revision);
        if (update.relations().hasAlbum()) {
            const auto& link = update.relations().album();
            if (link.hasId_proto() && link.id_proto() <= 0) {
                fail(QStringLiteral("Invalid album relation"));
                continue;
            }
            QVariant incoming = link.hasId_proto() ? QVariant(qint64(link.id_proto())) : QVariant();
            QVariant previous = entry->album ? QVariant(*entry->album) : QVariant();
            assign(
                key, *entry, Field::Album, revision, incoming, ! link.hasId_proto(), previous, [&] {
                    entry->album =
                        link.hasId_proto() ? std::optional<qint64>(link.id_proto()) : std::nullopt;
                    auto song = entry->data.value<model::Song>();
                    song.setAlbumId(entry->album.value_or(0));
                    entry->data = QVariant::fromValue(song);
                });
        }
    }
    for (const auto& update : batch.albums()) {
        model::Album data;
        data.setId_proto(update.id_proto());
        Key  key { Album, update.id_proto() };
        auto entry = prepare(key, update.stamp(), QVariant::fromValue(data));
        if (! entry) continue;
        metadata.template operator()<model::Album>(
            key, *entry, update.fields(), update.stamp().revision());
        state(key, *entry, update.state(), update.stamp().revision());
        if (update.relations().hasArtists())
            artists(key, *entry, update.relations().artists(), update.stamp().revision());
    }
    for (const auto& update : batch.artists()) {
        model::Artist data;
        data.setId_proto(update.id_proto());
        Key  key { Artist, update.id_proto() };
        auto entry = prepare(key, update.stamp(), QVariant::fromValue(data));
        if (! entry) continue;
        metadata.template operator()<model::Artist>(
            key, *entry, update.fields(), update.stamp().revision());
        state(key, *entry, update.state(), update.stamp().revision());
    }
    for (const auto& update : batch.mixes()) {
        model::Mix data;
        data.setId_proto(update.id_proto());
        Key  key { Mix, update.id_proto() };
        auto entry = prepare(key, update.stamp(), QVariant::fromValue(data));
        if (! entry) continue;
        metadata.template operator()<model::Mix>(
            key, *entry, update.fields(), update.stamp().revision());
    }
    for (const auto& removal : batch.removals()) {
        Key key = canonical({ int(removal.type()), removal.id_proto() });
        if (key.id <= 0 ||
            (key.type != Song && key.type != Album && key.type != Artist && key.type != Mix) ||
            removal.stamp().revision() == 0 || removal.stamp().owner() < 0) {
            fail(QStringLiteral("Invalid entity removal"));
            continue;
        }
        if (removal.stamp().revision() < m_floor) continue;
        const auto epoch = epochs.value(removal.stamp().owner(), 0);
        if (removal.stamp().epoch() < epoch) continue;
        if (removal.stamp().epoch() > epoch) {
            fail(QStringLiteral("Unannounced removal epoch"));
            continue;
        }
        auto    existing = staged.contains(key) ? &staged[key] : find(key);
        quint64 latest   = 0;
        if (existing)
            for (const auto& version : existing->versions)
                if (version) latest = std::max(latest, *version);
        if (latest > removal.stamp().revision()) continue;
        const bool hadEntry = existing != nullptr;
        removed.insert(key);
        staged.remove(key);
        statuses[key]   = Status::Deleted;
        tombstones[key] = std::max(tombstones.value(key), quint64(removal.stamp().revision()));
        if (hadEntry || ! m_tombstones.contains(key)) changed[key] = AllFields;
    }
    if (! result) return result;
    for (auto it = staged.cbegin(); it != staged.cend(); ++it)
        if (auto previous = find(it.key())) {
            auto&  fields    = changed[it.key()];
            Fields knowledge = 0;
            for (unsigned i = 0; i < unsigned(Field::Count); ++i)
                if (previous->known(Field(i)) != it->known(Field(i))) knowledge |= quint64(1) << i;
            constexpr auto metadata = (quint64(1) << unsigned(Field::Album)) - 1;
            if (previous->data == it->data && previous->nulls == it->nulls)
                fields &= ~metadata | knowledge;
            if (previous->artists == it->artists && ! (knowledge & bit(Field::Artists)))
                fields &= ~bit(Field::Artists);
            if (previous->album == it->album && ! (knowledge & bit(Field::Album)))
                fields &= ~bit(Field::Album);
            if (previous->favorite == it->favorite && ! (knowledge & bit(Field::Favorite)))
                fields &= ~bit(Field::Favorite);
        }
    result.stateChanges.clear();
    for (auto it = staged.cbegin(); it != staged.cend(); ++it)
        if (auto previous = find(it.key());
            previous && previous->favorite && previous->favorite != it->favorite)
            result.stateChanges.push_back(it.key());
    m_instance = batch.instance();
    if (m_epochs != epochs) ++m_scopeRevision;
    m_epochs     = std::move(epochs);
    m_tombstones = std::move(tombstones);
    for (auto key : removed) {
        m_entries.remove(key);
        rememberAbsence(key, statuses.value(key, Status::Invalidated));
    }
    if (m_tombstones.size() > m_tombstoneCapacity) {
        QList<quint64> revisions = m_tombstones.values();
        std::sort(revisions.begin(), revisions.end());
        m_floor = std::max(m_floor, revisions[revisions.size() - m_tombstoneCapacity - 1]);
        for (auto it = m_tombstones.begin(); it != m_tombstones.end();)
            if (it.value() <= m_floor)
                it = m_tombstones.erase(it);
            else
                ++it;
    }
    for (auto it = staged.begin(); it != staged.end(); ++it) {
        it->touched = ++m_clock;
        m_absent.remove(it.key());
        m_entries.insert(it.key(), std::move(it.value()));
    }
    rebuildDependencies();
    auto queue = changed.keys();
    for (qsizetype i = 0; i < queue.size(); ++i) {
        const auto key = queue[i];
        if (! (changed[key] & (bit(Field::Name) | bit(Field::Presence) | bit(Field::Deleted))))
            continue;
        for (const auto& dependency : m_dependents.value(key)) {
            const auto old = changed.value(dependency.key);
            changed[dependency.key] |= dependency.fields;
            if (! old) queue.push_back(dependency.key);
        }
    }
    for (auto it = changed.cbegin(); it != changed.cend(); ++it)
        if (it.value())
            result.changes.push_back(
                { it.key(), it.value(), statuses.value(it.key(), status(it.key())) });
    return result;
}

QList<Change> EntityStore::collect() {
    QSet<Key>  reachable;
    QList<Key> pending;
    for (auto it = m_pins.cbegin(); it != m_pins.cend(); ++it)
        if (it.value()) pending.push_back(it.key());
    for (qsizetype i = 0; i < pending.size(); ++i) {
        auto key = pending[i];
        if (reachable.contains(key)) continue;
        reachable.insert(key);
        if (auto entry = find(key)) {
            for (auto id : entry->artists) pending.push_back({ Artist, id });
            if (entry->album) pending.push_back({ Album, *entry->album });
        }
    }
    QList<Key> candidates;
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it)
        if (! reachable.contains(it.key())) candidates.push_back(it.key());
    std::sort(candidates.begin(), candidates.end(), [&](auto a, auto b) {
        return m_entries[a].touched < m_entries[b].touched;
    });
    QList<Change> changes;
    auto          used = bytes();
    for (auto key : candidates) {
        if (m_entries.size() <= m_capacity && used <= m_byteBudget) break;
        used -= entryBytes(m_entries.value(key));
        m_entries.remove(key);
        rememberAbsence(key, Status::Evicted);
        changes.push_back({ key, AllFields, Status::Evicted });
    }
    trimAbsences(m_absent, m_pins, m_tombstoneCapacity);
    rebuildDependencies();
    return changes;
}

qsizetype EntityStore::entryBytes(const Entry& entry) {
    qsizetype bytes = sizeof(Entry) + (entry.artists.size() + bool(entry.album)) *
                                          (sizeof(qint64) + sizeof(Change) + 32);
    auto      meta  = entry.data.metaType().metaObject();
    if (! meta) return bytes;
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        auto property = meta->property(i);
        if (property.metaType() == QMetaType::fromType<QString>())
            bytes +=
                property.readOnGadget(entry.data.constData()).toString().size() * sizeof(QChar);
        else if (property.metaType() == QMetaType::fromType<QStringList>())
            for (const auto& text : property.readOnGadget(entry.data.constData()).toStringList())
                bytes += sizeof(QString) + text.size() * sizeof(QChar);
    }
    return bytes;
}

void EntityStore::rememberAbsence(Key key, Status status) {
    m_absent.insert(key, status);
    trimAbsences(m_absent, m_pins, m_tombstoneCapacity);
}
void EntityStore::rebuildDependencies() {
    m_dependents.clear();
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it) {
        for (auto id : it->artists)
            m_dependents[{ Artist, id }].push_back({ it.key(), bit(Field::Artists) });
        if (it->album)
            m_dependents[{ Album, *it->album }].push_back({ it.key(), bit(Field::Album) });
    }
}

} // namespace qcm::store
