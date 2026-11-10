module;
#include <rstd/enum.hpp>
export module qcm.backend:db.query.input;
export import :pagination;
import :db.timestamp;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] TextCondition
{
    Unspecified,
    Is,
    IsNot,
    Contains,
    ContainsNot
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] NumberCondition
{
    Unspecified,
    Equal,
    EqualNot,
    Less,
    LessEqual,
    Greater,
    GreaterEqual
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] DateCondition
{
    Unspecified,
    Before,
    After,
    Null,
    NotNull
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] TypeCondition
{
    Unspecified,
    Is,
    IsNot
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] LogicOp
{
    Unspecified,
    And,
    Or
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] AlbumSort
{
    Title,
    SortTitle,
    Year,
    PublishTime,
    TrackCount,
    AddedTime,
    DiscCount,
    LastPlayedAt,
    Random = 99
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] ArtistSort
{
    Name,
    SortName,
    AlbumCount,
    MusicCount
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] SongSort
{
    TrackNumber,
    Title,
    SortTitle,
    PublishTime,
    Duration,
    Popularity,
    Random = 99
};
struct TextFilter {
    String        value;
    TextCondition condition { TextCondition::Unspecified };
};
struct NumberFilter {
    i64             value;
    NumberCondition condition { NumberCondition::Unspecified };
};
struct DateFilter {
    Timestamp     value;
    DateCondition condition { DateCondition::Unspecified };
};
struct TypeFilter {
    i64           value;
    TypeCondition condition { TypeCondition::Unspecified };
};
struct TypeTextFilter {
    String        value;
    TypeCondition condition { TypeCondition::Unspecified };
};
struct FilterLogic {
    i32     group_a;
    i32     group_b;
    LogicOp operation { LogicOp::Unspecified };
};
class AlbumFilter {
    RSTD_ENUM(
        AlbumFilter,
        (Name, (TextFilter filter;)),
        (TrackCount, (NumberFilter filter;)),
        (ArtistName, (TextFilter filter;)),
        (ArtistId, (i64 id;)),
        (AlbumArtistId, (i64 id;)),
        (Year, (NumberFilter filter;)),
        (AddedAt, (DateFilter filter;)),
        (Duration, (NumberFilter filter;)),
        (Type, (TypeFilter filter;)),
        (DiscCount, (NumberFilter filter;)),
        (LastPlayedAt, (DateFilter filter;))
    )
};
class ArtistFilter {
    RSTD_ENUM(
        ArtistFilter,
        (Name, (TextFilter filter;)),
        (AlbumName, (TextFilter filter;)),
        (AddedAt, (DateFilter filter;))
    )
};
class SongFilter {
    RSTD_ENUM(SongFilter, (AlbumId, (i64 id;)), (MixId, (i64 id;)))
};
class MixFilter {
    RSTD_ENUM(
        MixFilter,
        (Name, (TextFilter filter;)),
        (TrackCount, (NumberFilter filter;)),
        (Type, (TypeFilter filter;))
    )
};
class RemoteMixFilter {
    RSTD_ENUM(
        RemoteMixFilter,
        (Name, (TextFilter filter;)),
        (TrackCount, (NumberFilter filter;)),
        (Type, (TypeTextFilter filter;)),
        (LocalType, (TypeFilter filter;))
    )
};
template<class T>
struct GroupedFilter {
    i32 group;
    T   value;
};
template<class T>
struct QueryFilters {
    Vec<GroupedFilter<T>> values;
    Vec<FilterLogic>      logics;
};
struct AlbumQuery {
    Vec<i64>                  libraries;
    QueryFilters<AlbumFilter> filters;
    AlbumSort                 sort { AlbumSort::Title };
    bool                      ascending;
    Page                      page;
};
struct ArtistQuery {
    Vec<i64>                   libraries;
    QueryFilters<ArtistFilter> filters;
    ArtistSort                 sort { ArtistSort::Name };
    bool                       ascending;
    Page                       page;
};
struct AlbumSongQuery {
    AlbumQuery albums;
    SongSort   sort { SongSort::TrackNumber };
    bool       ascending;
};
struct FilteredSongQuery {
    QueryFilters<SongFilter> filters;
    SongSort                 sort { SongSort::TrackNumber };
    bool                     ascending;
};
class SongIdsQuery {
    RSTD_ENUM(SongIdsQuery, (Albums, (AlbumSongQuery query;)), (Songs, (FilteredSongQuery query;)))
};
struct MixQuery {
    QueryFilters<MixFilter> filters;
    Page                    page;
};
struct RemoteMixQuery {
    QueryFilters<RemoteMixFilter> filters;
    Page                          page;
};
struct ArtistAlbumQuery {
    i64       artist_id;
    AlbumSort sort { AlbumSort::PublishTime };
    bool      ascending;
    Page      page;
};
struct IdQueryPage {
    Vec<i64> ids;
    i64      total {};
};
enum class [[pmacro::derive("rstd::EnumIter", "rstd::TryFrom")]] SearchKind
{
    Album,
    Artist,
    Song
};
struct SearchQuery {
    SearchKind kind;
    String     text;
    Vec<i64>   libraries;
    Page       page;
};
} // namespace qcm::backend
