export module qcm.backend:convert;
export import :protocol.types;
export import rstd;
import :model.album;
import :model.artist;
import :model.home;
import :model.library;
import :model.mix_model;
import :model.song;
import :db.timestamp;
using namespace rstd::prelude;

namespace qcm::backend::conversion_detail
{
using TimeValue = ::p_qcm::p_msg::p_entity::M_TimeValue;
using TextValue = ::p_qcm::p_msg::p_entity::M_TextValue;

auto timestamp_millis(Timestamp timestamp) -> Box<p_google::p_protobuf::M_Timestamp> {
    auto ms        = timestamp.as_millis().to_primitive();
    auto seconds   = ms / 1000;
    auto remainder = ms % 1000;
    if (remainder < 0) {
        --seconds;
        remainder += 1000;
    }
    auto stamp       = Box<p_google::p_protobuf::M_Timestamp>::make();
    stamp->f_seconds = i64(seconds);
    stamp->f_nanos   = i32(static_cast<int>(remainder * 1000000));
    return stamp;
}

auto optional_timestamp_millis(Option<Timestamp> millis)
    -> Option<Box<p_google::p_protobuf::M_Timestamp>> {
    if (millis.is_none()) return None();
    return Some(timestamp_millis(*millis));
}

auto time_value(Option<Timestamp> millis) -> Box<TimeValue> {
    auto value     = Box<TimeValue>::make();
    value->f_value = optional_timestamp_millis(millis);
    return value;
}

auto text_value(Option<String> value) -> Box<TextValue> {
    auto result     = Box<TextValue>::make();
    result->f_value = rstd::move(value);
    return result;
}

auto song_tags(const Song& song) -> Vec<String> {
    Vec<String> result;
    if (auto values = song.tags.as_array(); values.is_some()) {
        for (const auto& tag : (**values).as_slice()) {
            if (auto text = tag.as_str(); text.is_some()) result.push(String::make(*text));
        }
    }
    return result;
}

auto song_tags(Song&& song) -> Vec<String> {
    Vec<String> result;
    if (auto values = song.tags.as_array_mut(); values.is_some()) {
        for (auto& tag : **values) {
            if (tag.is_String()) result.push(rstd::move(tag.as_String().value));
        }
    }
    return result;
}
} // namespace qcm::backend::conversion_detail

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::Song>, Box<p_qcm::p_msg::p_entity::M_SongFields>> {
    static auto from(qcm::backend::Song song) -> Box<p_qcm::p_msg::p_entity::M_SongFields> {
        using namespace qcm::backend::conversion_detail;
        auto fields            = Box<::p_qcm::p_msg::p_entity::M_SongFields>::make();
        fields->f_name         = Some(rstd::move(song.name));
        fields->f_sort_name    = Some(text_value(rstd::move(song.sort_name)));
        fields->f_track_number = Some(i32(song.track_number));
        fields->f_disc_number  = Some(i32(song.disc_number));
        fields->f_duration     = Some(f64(static_cast<double>(song.duration.to_primitive())));
        fields->f_can_play     = Some(bool(song.can_play));
        fields->f_publish_time = Some(time_value(song.publish_time));
        fields->f_popularity   = Some(f64(song.popularity));
        auto tags              = Box<::p_qcm::p_msg::p_entity::M_StringList>::make();
        tags->f_values         = song_tags(rstd::move(song));
        fields->f_tags         = Some(rstd::move(tags));
        return fields;
    }
};

template<>
struct Impl<convert::From<qcm::backend::Album>, Box<p_qcm::p_msg::p_entity::M_AlbumFields>> {
    static auto from(qcm::backend::Album album) -> Box<p_qcm::p_msg::p_entity::M_AlbumFields> {
        using namespace qcm::backend::conversion_detail;
        auto fields            = Box<::p_qcm::p_msg::p_entity::M_AlbumFields>::make();
        fields->f_name         = Some(rstd::move(album.name));
        fields->f_sort_name    = Some(text_value(rstd::move(album.sort_name)));
        fields->f_type         = Some(i32(static_cast<int>(album.type)));
        fields->f_duration     = Some(i64(album.duration));
        fields->f_track_count  = Some(i32(album.track_count));
        fields->f_disc_count   = Some(i32(album.disc_count));
        fields->f_description  = Some(text_value(rstd::move(album.description)));
        fields->f_company      = Some(text_value(rstd::move(album.company)));
        fields->f_language     = Some(text_value(rstd::move(album.language)));
        fields->f_publish_time = Some(time_value(album.publish_time));
        fields->f_added_at     = Some(time_value(album.added_at));
        return fields;
    }
};

template<>
struct Impl<convert::From<qcm::backend::Artist>, Box<p_qcm::p_msg::p_entity::M_ArtistFields>> {
    static auto from(qcm::backend::Artist artist) -> Box<p_qcm::p_msg::p_entity::M_ArtistFields> {
        using namespace qcm::backend::conversion_detail;
        auto fields           = Box<::p_qcm::p_msg::p_entity::M_ArtistFields>::make();
        fields->f_name        = Some(rstd::move(artist.name));
        fields->f_sort_name   = Some(text_value(rstd::move(artist.sort_name)));
        fields->f_description = Some(rstd::move(artist.description));
        fields->f_album_count = Some(i32(artist.album_count));
        fields->f_music_count = Some(i32(artist.music_count));
        fields->f_added_at    = Some(time_value(artist.added_at));
        return fields;
    }
};

template<>
struct Impl<convert::From<qcm::backend::Mix>, Box<p_qcm::p_msg::p_entity::M_MixFields>> {
    static auto from(qcm::backend::Mix mix) -> Box<p_qcm::p_msg::p_entity::M_MixFields> {
        using namespace qcm::backend::conversion_detail;
        auto fields           = Box<::p_qcm::p_msg::p_entity::M_MixFields>::make();
        fields->f_name        = Some(rstd::move(mix.name));
        fields->f_sort_name   = Some(text_value(rstd::move(mix.sort_name)));
        fields->f_track_count = Some(i32(mix.track_count));
        fields->f_description = Some(rstd::move(mix.description));
        fields->f_mix_type    = Some(i32(static_cast<int>(mix.mix_type)));
        fields->f_update_at   = Some(time_value(Some(mix.update_at)));
        return fields;
    }
};

template<>
struct Impl<convert::From<ref<qcm::backend::Song>>, Box<p_qcm::p_msg::p_model::M_Song>> {
    static auto from(ref<qcm::backend::Song> value) -> Box<p_qcm::p_msg::p_model::M_Song> {
        using namespace qcm::backend::conversion_detail;
        const auto& model   = *value;
        auto        result  = Box<::p_qcm::p_msg::p_model::M_Song>::make();
        auto        id      = Box<::p_qcm::p_msg::p_id::M_SongId>::make();
        id->f_value         = model.id;
        result->f_id        = Some(rstd::move(id));
        result->f_name      = model.name.clone();
        result->f_sort_name = model.sort_name.clone().unwrap_or(String::make());
        if (model.album_id.is_some()) {
            auto album         = Box<::p_qcm::p_msg::p_id::M_AlbumId>::make();
            album->f_value     = *model.album_id;
            result->f_album_id = Some(rstd::move(album));
        }
        result->f_track_number = model.track_number;
        result->f_disc_number  = model.disc_number;
        result->f_duration     = f64(static_cast<double>(model.duration.to_primitive()));
        result->f_can_play     = model.can_play;
        result->f_popularity   = model.popularity;
        result->f_publish_time = optional_timestamp_millis(model.publish_time);
        result->f_tags         = song_tags(model);
        return result;
    }
};
template<>
struct Impl<convert::From<ref<qcm::backend::Album>>, Box<p_qcm::p_msg::p_model::M_Album>> {
    static auto from(ref<qcm::backend::Album> value) -> Box<p_qcm::p_msg::p_model::M_Album> {
        using namespace qcm::backend::conversion_detail;
        const auto& model      = *value;
        auto        result     = Box<::p_qcm::p_msg::p_model::M_Album>::make();
        auto        id         = Box<::p_qcm::p_msg::p_id::M_AlbumId>::make();
        id->f_value            = model.id;
        result->f_id           = Some(rstd::move(id));
        result->f_name         = model.name.clone();
        result->f_sort_name    = model.sort_name.clone().unwrap_or(String::make());
        result->f_type         = i32(static_cast<int>(model.type));
        result->f_duration     = model.duration;
        result->f_track_count  = model.track_count;
        result->f_disc_count   = model.disc_count;
        result->f_description  = model.description.clone().unwrap_or(String::make());
        result->f_company      = model.company.clone().unwrap_or(String::make());
        result->f_language     = model.language.clone().unwrap_or(String::make());
        result->f_publish_time = optional_timestamp_millis(model.publish_time);
        result->f_added_at     = optional_timestamp_millis(model.added_at);
        return result;
    }
};
template<>
struct Impl<convert::From<ref<qcm::backend::Artist>>, Box<p_qcm::p_msg::p_model::M_Artist>> {
    static auto from(ref<qcm::backend::Artist> value) -> Box<p_qcm::p_msg::p_model::M_Artist> {
        using namespace qcm::backend::conversion_detail;
        const auto& model     = *value;
        auto        result    = Box<::p_qcm::p_msg::p_model::M_Artist>::make();
        auto        id        = Box<::p_qcm::p_msg::p_id::M_ArtistId>::make();
        id->f_value           = model.id;
        result->f_id          = Some(rstd::move(id));
        result->f_name        = model.name.clone();
        result->f_sort_name   = model.sort_name.clone().unwrap_or(String::make());
        result->f_description = model.description.clone();
        result->f_album_count = model.album_count;
        result->f_music_count = model.music_count;
        result->f_added_at    = optional_timestamp_millis(model.added_at);
        return result;
    }
};
template<>
struct Impl<convert::From<ref<qcm::backend::Mix>>, Box<p_qcm::p_msg::p_model::M_Mix>> {
    static auto from(ref<qcm::backend::Mix> value) -> Box<p_qcm::p_msg::p_model::M_Mix> {
        using namespace qcm::backend::conversion_detail;
        const auto& model     = *value;
        auto        result    = Box<::p_qcm::p_msg::p_model::M_Mix>::make();
        auto        id        = Box<::p_qcm::p_msg::p_id::M_MixId>::make();
        id->f_value           = model.id;
        result->f_id          = Some(rstd::move(id));
        result->f_name        = model.name.clone();
        result->f_sort_name   = model.sort_name.clone().unwrap_or(String::make());
        result->f_track_count = model.track_count;
        result->f_mix_type    = i32(static_cast<int>(model.mix_type));
        result->f_description = model.description.clone();
        if (model.update_at > qcm::backend::Timestamp()) {
            result->f_update_at = Some(timestamp_millis(model.update_at));
        }
        return result;
    }
};
template<>
struct Impl<convert::From<qcm::backend::Timestamp>, Box<p_google::p_protobuf::M_Timestamp>> {
    static auto from(qcm::backend::Timestamp value) -> Box<p_google::p_protobuf::M_Timestamp> {
        return qcm::backend::conversion_detail::timestamp_millis(value);
    }
};
template<>
struct Impl<convert::From<qcm::backend::DatabaseDateTime>, Box<p_google::p_protobuf::M_Timestamp>> {
    static auto from(qcm::backend::DatabaseDateTime value)
        -> Box<p_google::p_protobuf::M_Timestamp> {
        auto stamp       = Box<p_google::p_protobuf::M_Timestamp>::make();
        stamp->f_seconds = value.seconds;
        stamp->f_nanos   = i32(static_cast<int>(value.nanoseconds));
        return stamp;
    }
};
template<>
struct Impl<convert::From<qcm::backend::Library>, Box<p_qcm::p_msg::p_model::M_Library>> {
    static auto from(qcm::backend::Library value) -> Box<p_qcm::p_msg::p_model::M_Library> {
        auto result           = Box<p_qcm::p_msg::p_model::M_Library>::make();
        auto id               = Box<p_qcm::p_msg::p_id::M_LibraryId>::make();
        id->f_value           = value.id;
        result->f_library_id  = Some(rstd::move(id));
        auto provider         = Box<p_qcm::p_msg::p_id::M_ProviderId>::make();
        provider->f_value     = value.provider_id;
        result->f_provider_id = Some(rstd::move(provider));
        result->f_name        = rstd::move(value.name);
        result->f_native_id   = rstd::move(value.native_id);
        result->f_edit_time =
            Some(rstd::into<Box<p_google::p_protobuf::M_Timestamp>>(value.edit_time));
        return result;
    }
};
template<>
struct Impl<convert::From<qcm::backend::HomeBlock>, Box<p_qcm::p_msg::p_model::M_HomeBlock>> {
    static auto from(qcm::backend::HomeBlock value) -> Box<p_qcm::p_msg::p_model::M_HomeBlock> {
        auto block           = Box<p_qcm::p_msg::p_model::M_HomeBlock>::make();
        block->f_native_id   = rstd::move(value.native_id);
        block->f_title       = rstd::move(value.title);
        block->f_subtitle    = rstd::move(value.subtitle).unwrap_or(String::make());
        block->f_description = rstd::move(value.description).unwrap_or(String::make());
        block->f_style       = i32(static_cast<int>(value.style));
        block->f_item_type   = i32(static_cast<int>(value.item_type));
        return block;
    }
};
template<>
struct Impl<convert::From<ref<qcm::backend::HomeBlock>>, Box<p_qcm::p_msg::p_model::M_HomeBlock>> {
    static auto from(ref<qcm::backend::HomeBlock> value)
        -> Box<p_qcm::p_msg::p_model::M_HomeBlock> {
        auto block           = Box<p_qcm::p_msg::p_model::M_HomeBlock>::make();
        block->f_native_id   = value->native_id.clone();
        block->f_title       = value->title.clone();
        block->f_subtitle    = value->subtitle.clone().unwrap_or(String::make());
        block->f_description = value->description.clone().unwrap_or(String::make());
        block->f_style       = i32(static_cast<int>(value->style));
        block->f_item_type   = i32(static_cast<int>(value->item_type));
        return block;
    }
};
} // namespace rstd
