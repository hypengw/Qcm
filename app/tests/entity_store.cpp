#include "../src/message/include/Qcm/message/entity_store_impl.hpp"
#include "../src/message/include/Qcm/message/load_pool.hpp"
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtProtobuf/QProtobufSerializer>
#include <QtQml/QJSEngine>
#include <source_location>

using namespace qcm::store;

static void check(bool condition, std::source_location location = std::source_location::current()) {
    if (! condition)
        qFatal("entity-store check failed at %s:%u", location.file_name(), location.line());
}
static wire::EntityBatch batch() {
    wire::EntityBatch value;
    value.setProtocol(1);
    value.setInstance("backend-a");
    return value;
}
static wire::ReadStamp stamp(quint64 revision, qint64 owner = 1) {
    wire::ReadStamp value;
    value.setOwner(owner);
    value.setRevision(revision);
    return value;
}
static wire::SongUpdate song(quint64 revision) {
    wire::SongUpdate value;
    value.setId_proto(Q_INT64_C(9007199254740993));
    value.setStamp(stamp(revision));
    return value;
}
static wire::EntityBatch withSong(wire::SongUpdate song) {
    auto value = batch();
    value.setSongs({ song });
    return value;
}

static void dependenciesAndBudgets() {
    const Key           key { Song, Q_INT64_C(9007199254740993) };
    EntityStore         store;
    auto                value = song(1);
    wire::SongRelations relations;
    wire::EntityIds     ids;
    ids.setIds({ 2 });
    relations.setArtists(ids);
    value.setRelations(relations);
    auto               incoming = withSong(value);
    wire::ArtistUpdate artist;
    artist.setId_proto(2);
    artist.setStamp(stamp(1));
    wire::ArtistFields fields;
    fields.setName("artist");
    fields.setDescription("detail");
    artist.setFields(fields);
    incoming.setArtists({ artist });
    check(bool(store.apply(incoming)));
    auto update = batch();
    artist.setStamp(stamp(2));
    fields = {};
    fields.setName("renamed");
    artist.setFields(fields);
    update.setArtists({ artist });
    auto changed = store.apply(update);
    check(bool(changed));
    check(std::any_of(changed.changes.begin(), changed.changes.end(), [&](const auto& c) {
        return c.key == key && c.fields == bit(Field::Artists);
    }));
    check(store.value<model::Artist>({ Artist, 2 }).description() == "detail");
    value = song(3);
    ids.setIds({ 3 });
    relations.setArtists(ids);
    value.setRelations(relations);
    check(bool(store.apply(withSong(value))));
    artist.setStamp(stamp(4));
    fields.setName("unreferenced");
    artist.setFields(fields);
    update.setArtists({ artist });
    changed = store.apply(update);
    check(changed.changes.size() == 1 && changed.changes[0].key == Key { Artist, 2 });
    store.pin(key);
    store.setCapacity(0);
    store.setByteBudget(0);
    store.collect();
    check(store.find(key) && ! store.find({ Artist, 2 }));
    check(store.status({ Artist, 2 }) == Status::Evicted);
    store.unpin(key);
    store.collect();
    check(store.size() == 0);

    EntityStore bounded;
    bounded.setTombstoneCapacity(2);
    auto                 deleted = batch();
    QList<wire::Removal> removals;
    for (qint64 id = 1; id <= 10; ++id) {
        wire::Removal removal;
        removal.setId_proto(id);
        removal.setType(model::ItemTypeGadget::ItemType::ITEM_TYPE_SONG);
        removal.setStamp(stamp(id + 10));
        removals.push_back(removal);
    }
    deleted.setRemovals(removals);
    check(bool(bounded.apply(deleted)));
    value = song(1);
    value.setId_proto(1);
    check(bool(bounded.apply(withSong(value))) && ! bounded.find({ Song, 1 }));
    value = song(30);
    value.setId_proto(1);
    check(bool(bounded.apply(withSong(value))) && bounded.find({ Song, 1 }));
    EntityStore sameRevision;
    sameRevision.setTombstoneCapacity(2);
    for (auto& removal : removals) removal.setStamp(stamp(20));
    deleted.setRemovals(removals);
    check(bool(sameRevision.apply(deleted)));
    value = song(20);
    value.setId_proto(100);
    check(bool(sameRevision.apply(withSong(value))) && sameRevision.find({ Song, 100 }));
    value = song(19);
    value.setId_proto(1);
    check(bool(sameRevision.apply(withSong(value))) && ! sameRevision.find({ Song, 1 }));
    EntityStore observed;
    observed.setTombstoneCapacity(0);
    observed.pin({ Song, 1 });
    for (qint64 id = 1; id <= 100; ++id) {
        wire::Removal removal;
        removal.setId_proto(id);
        removal.setType(model::ItemTypeGadget::ItemType::ITEM_TYPE_SONG);
        removal.setStamp(stamp(30));
        deleted.setRemovals({ removal });
        check(bool(observed.apply(deleted)));
    }
    check(observed.status({ Song, 1 }) == Status::Deleted);
}

static void nullableAndEpochs() {
    EntityStore       store;
    const Key         key { Album, 4 };
    wire::AlbumUpdate album;
    album.setId_proto(4);
    album.setStamp(stamp(1));
    wire::AlbumFields fields;
    wire::TextValue   text;
    text.setValue("detail");
    fields.setDescription(text);
    fields.setDuration(Q_INT64_C(9007199254740993));
    fields.setTrackCount(0);
    fields.setType(model::AlbumTypeGadget::AlbumType::ALBUM_TYPE_COMPILATION);
    wire::TimeValue             published;
    google::protobuf::Timestamp time;
    time.setSeconds(123);
    published.setValue(time);
    fields.setPublishTime(published);
    album.setFields(fields);
    auto incoming = batch();
    incoming.setAlbums({ album });
    check(bool(store.apply(incoming)));
    check(store.value<model::Album>(key).duration() == Q_INT64_C(9007199254740993));
    check(store.value<model::Album>(key).type() ==
          model::AlbumTypeGadget::AlbumType::ALBUM_TYPE_COMPILATION);
    check(store.value<model::Album>(key).publishTime().seconds() == 123);
    album.setStamp(stamp(2));
    fields = {};
    fields.setDescription({});
    fields.setPublishTime({});
    album.setFields(fields);
    incoming.setAlbums({ album });
    check(bool(store.apply(incoming)));
    check(store.find(key)->known(Field::Description) &&
          (store.find(key)->nulls & bit(Field::Description)));
    album.setStamp(stamp(3));
    text.setValue("");
    fields.setDescription(text);
    album.setFields(fields);
    incoming.setAlbums({ album });
    check(bool(store.apply(incoming)));
    check(! (store.find(key)->nulls & bit(Field::Description)));
    store.markStale();
    check(store.status(key) == Status::Stale);
    check(bool(store.apply(incoming)) && store.status(key) == Status::Ready);
    auto invalidate = batch();
    auto next       = stamp(4);
    next.setEpoch(1);
    invalidate.setInvalidations({ next });
    check(bool(store.apply(invalidate)));
    check(! store.find(key) && store.status(key) == Status::Invalidated);
    check(bool(store.apply(incoming)) && ! store.find(key));
    album.setStamp(next);
    incoming.setAlbums({ album });
    check(bool(store.apply(incoming)) && store.find(key));
    auto invalid = stamp(5);
    invalid.setEpoch(2);
    album.setStamp(invalid);
    incoming.setAlbums({ album });
    check(! store.apply(incoming));
    check(store.find(key)->epoch == 1);
}

static void loadCoalescing() {
    struct Flight {
        int consumers {};
    };
    LoadPool<QString, Flight> pool(2);
    int                       requests = 0;
    auto                      start    = [&] {
        ++requests;
        return std::make_shared<Flight>();
    };
    auto [first, leader] = pool.acquire("epoch1/song/full/1", start);
    check(leader);
    auto [second, shared] = pool.acquire("epoch1/song/full/1", start);
    check(! shared && first == second && requests == 1);
    first->consumers = 2;
    --second->consumers;
    check(pool.find("epoch1/song/full/1") == first && first->consumers == 1);
    auto [different, other] = pool.acquire("epoch2/song/full/1", start);
    check(other && different != first);
    check(! pool.acquire("epoch2/album/full/1", start).first);
    check(pool.take("epoch1/song/full/1", first) == first);
    auto [replacement, restarted] = pool.acquire("epoch1/song/full/1", start);
    check(restarted);
    check(! pool.take("epoch1/song/full/1", first));
    check(pool.find("epoch1/song/full/1") == replacement);
    check(pool.clear().size() == 2 && ! pool.find("epoch2/song/full/1"));
}

static void libraryBudget() {
    EntityStore               store;
    auto                      incoming = batch();
    QList<wire::SongUpdate>   songs;
    QList<wire::ArtistUpdate> artists;
    for (qint64 id = 1; id <= 10000; ++id) {
        auto update = song(1);
        update.setId_proto(id);
        wire::SongFields fields;
        fields.setName(QStringLiteral("song %1").arg(id));
        update.setFields(fields);
        wire::SongRelations relations;
        wire::EntityIds     ids;
        ids.setIds({ 100001 + id % 1000 });
        relations.setArtists(ids);
        update.setRelations(relations);
        songs.push_back(update);
    }
    for (qint64 id = 100001; id <= 101000; ++id) {
        wire::ArtistUpdate update;
        update.setId_proto(id);
        update.setStamp(stamp(1));
        wire::ArtistFields fields;
        fields.setName(QStringLiteral("artist %1").arg(id));
        update.setFields(fields);
        artists.push_back(update);
    }
    incoming.setSongs(songs);
    incoming.setArtists(artists);
    QElapsedTimer timer;
    timer.start();
    check(bool(store.apply(incoming)));
    const auto applyMs = timer.elapsed();
    for (qint64 id = 1; id <= 100; ++id) store.pin({ Song, id });
    store.setCapacity(2000);
    store.collect();
    check(store.size() <= 2000);
    for (qint64 id = 1; id <= 100; ++id) {
        check(store.find({ Song, id }));
        check(store.find({ Artist, 100001 + id % 1000 }));
        store.unpin({ Song, id });
    }
    store.setCapacity(0);
    store.collect();
    check(store.size() == 0);
    qInfo("entity-store sample: 10000 songs + 1000 artists, apply %lld ms, total %lld ms",
          applyMs,
          timer.elapsed());
}

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    const Key        key { Song, Q_INT64_C(9007199254740993) };
    EntityStore      store;
    auto             first = song(1);
    wire::SongFields fields;
    fields.setName("first");
    fields.setTrackNumber(0);
    fields.setCanPlay(false);
    fields.setTags({});
    first.setFields(fields);
    wire::SongRelations relations;
    wire::EntityIds     artists;
    artists.setIds({ 2, 3 });
    relations.setArtists(artists);
    wire::AlbumLink album;
    album.setId_proto(4);
    relations.setAlbum(album);
    first.setRelations(relations);
    auto                serialized = withSong(first);
    QProtobufSerializer serializer;
    wire::EntityBatch   decoded;
    check(decoded.deserialize(&serializer, serialized.serialize(&serializer)));
    check(decoded.songs()[0].id_proto() == key.id);
    QJSEngine     javascript;
    model::Artist artistId;
    artistId.setId_proto(key.id);
    javascript.globalObject().setProperty("artist", javascript.toScriptValue(artistId));
    auto jsId = javascript.evaluate("(() => artist.id_proto)()");
    check(! jsId.isError());
    check(qjsvalue_cast<QtProtobuf::int64>(jsId) == key.id);
    check(bool(store.apply(decoded)));
    check(store.find(key)->known(Field::CanPlay) && ! store.value<model::Song>(key).canPlay());
    check(store.find(key)->artists == QList<qint64>({ 2, 3 }));
    check(store.find(key)->album == 4);
    check(store.find(key)->known(Field::Tags) && store.value<model::Song>(key).tags().isEmpty());

    auto detail = song(3);
    fields      = {};
    fields.setDuration(5000);
    detail.setFields(fields);
    check(bool(store.apply(withSong(detail))));
    check(store.value<model::Song>(key).name() == "first");
    check(store.find(key)->artists.size() == 2);
    auto old = song(2);
    fields   = {};
    fields.setDuration(10);
    fields.setDiscNumber(2);
    old.setFields(fields);
    check(bool(store.apply(withSong(old))));
    check(store.value<model::Song>(key).duration() == 5000 &&
          store.value<model::Song>(key).discNumber() == 2);
    check(store.apply(withSong(detail)).changes.isEmpty());

    auto conflict = song(3);
    fields        = {};
    fields.setDuration(1);
    conflict.setFields(fields);
    auto bad = withSong(conflict);
    bad.setSongs({ song(4), conflict });
    check(! store.apply(bad));
    check(store.value<model::Song>(key).duration() == 5000);
    auto             roundTrip = withSong(song(4));
    auto             temporary = song(4);
    wire::SongFields temporaryFields;
    temporaryFields.setName("temporary");
    temporary.setFields(temporaryFields);
    auto restored = song(5);
    temporaryFields.setName("first");
    restored.setFields(temporaryFields);
    roundTrip.setSongs({ temporary, restored });
    check(store.apply(roundTrip).changes.isEmpty());
    auto            favorite = song(6);
    wire::ItemState state;
    state.setFavorite(true);
    favorite.setState(state);
    check(bool(store.apply(withSong(favorite))));
    old = song(4);
    state.setFavorite(false);
    old.setState(state);
    check(bool(store.apply(withSong(old))) && store.find(key)->favorite == true);
    favorite = song(7);
    state.setFavorite(false);
    favorite.setState(state);
    check(bool(store.apply(withSong(favorite))) && store.find(key)->favorite == false);

    auto clear = song(8);
    relations  = {};
    relations.setArtists({});
    relations.setAlbum({});
    clear.setRelations(relations);
    check(bool(store.apply(withSong(clear))));
    check(store.find(key)->artists.isEmpty() && ! store.find(key)->album);
    check(store.find(key)->known(Field::Artists) && store.find(key)->known(Field::Album));
    auto wrongInstance = batch();
    wrongInstance.setInstance("backend-b");
    check(! store.apply(wrongInstance));

    wire::Removal removal;
    removal.setId_proto(key.id);
    removal.setType(model::ItemTypeGadget::ItemType::ITEM_TYPE_SONG);
    removal.setStamp(stamp(9));
    auto deleted = batch();
    deleted.setRemovals({ removal });
    check(bool(store.apply(deleted)));
    check(! store.find(key));
    check(bool(store.apply(withSong(first))) && ! store.find(key));
    check(bool(store.apply(withSong(song(10)))) && store.find(key));
    store.pin(key);
    store.setCapacity(0);
    check(store.collect().isEmpty());
    store.unpin(key);
    check(store.collect().size() == 1 && ! store.find(key));
    libraryBudget();
    loadCoalescing();
    dependenciesAndBudgets();
    nullableAndEpochs();
    qInfo("entity-store: presence, partial, version, atomicity, deletion and pin checks passed");
}
