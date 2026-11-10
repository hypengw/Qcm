export module qcm.backend:resource.schema;
export import liorm;
export import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::resource_cache::cache_db
{
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("versions"), primary("versions_pk"))]] VersionRecord {
    [[pmacro::helper("liorm::table", storage(integer), primary)]] i64 id;
    [[pmacro::helper("liorm::table", storage(text))]] String          resource_key;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          provider;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          kind;
    [[pmacro::helper("liorm::table", storage(text))]] String          generation;
    [[pmacro::helper("liorm::table", storage(text))]] String          meta;
    [[pmacro::helper("liorm::table", storage(blob))]] Vec<u8>         blocks;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          last_use;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          obsolete;
};
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("segments"), primary("segments_pk"))]] SegmentRecord {
    [[pmacro::helper("liorm::table", storage(integer), primary)]] i64 version_id;
    [[pmacro::helper("liorm::table", storage(integer), primary)]] i64 number;
    [[pmacro::helper("liorm::table", storage(text))]] String          token;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          length;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          last_use;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          deleting;
    // SQLite's existing rowid breaks eviction ties; no stored column is added.
    [[pmacro::helper("liorm::table", storage(integer))]] i64 rowid;
};
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("packs"), primary("packs_pk"))]] PackRecord {
    [[pmacro::helper("liorm::table", storage(text), primary)]] String token;
    [[pmacro::helper("liorm::table", storage(text))]] String          group_key;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          provider;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          length;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          last_use;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          deleting;
};
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("objects"), primary("objects_pk"))]] ObjectRecord {
    [[pmacro::helper("liorm::table", storage(text), primary)]] String resource_key;
    [[pmacro::helper("liorm::table", storage(text))]] String          pack;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          offset;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          length;
    [[pmacro::helper("liorm::table", storage(blob))]] Vec<u8>         digest;
    [[pmacro::helper("liorm::table", storage(text))]] String          meta;
};
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("complete_files"), primary("complete_files_pk"))]]
CompleteFileRecord {
    [[pmacro::helper("liorm::table", storage(integer), primary)]] i64           version_id;
    [[pmacro::helper("liorm::table", storage(integer))]] i64                    ready;
    [[pmacro::helper("liorm::table", storage(integer))]] i64                    length;
    [[pmacro::helper("liorm::table", storage(blob), nullable)]] Option<Vec<u8>> digest;
    [[pmacro::helper("liorm::table", storage(integer))]] i64                    retry_at;
    [[pmacro::helper("liorm::table", storage(integer))]] i64                    format;
};
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("file_leases"), primary("file_leases_pk"))]]
FileLeaseRecord {
    [[pmacro::helper("liorm::table", storage(text), primary)]] String token;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          version_id;
    [[pmacro::helper("liorm::table", storage(text))]] String          client;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          expires;
};
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("file_clock"), primary("file_clock_pk"))]]
FileClockRecord {
    [[pmacro::helper("liorm::table", storage(integer), primary)]] i64 id;
    [[pmacro::helper("liorm::table", storage(integer))]] i64          time;
};
inline constexpr auto schema = liorm::schema::define([](liorm::schema::SchemaBuilder& b) {
    liorm::schema::register_table<VersionRecord>(b);
    liorm::schema::register_table<SegmentRecord>(b);
    liorm::schema::register_table<PackRecord>(b);
    liorm::schema::register_table<ObjectRecord>(b);
    liorm::schema::register_table<CompleteFileRecord>(b);
    liorm::schema::register_table<FileLeaseRecord>(b);
    liorm::schema::register_table<FileClockRecord>(b);
});
} // namespace qcm::backend::resource_cache::cache_db
