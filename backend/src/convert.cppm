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

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::Song>, Box<p_qcm::p_msg::p_entity::M_SongFields>> {
    static auto from(qcm::backend::Song song) -> Box<p_qcm::p_msg::p_entity::M_SongFields>;
};

template<>
struct Impl<convert::From<qcm::backend::Album>, Box<p_qcm::p_msg::p_entity::M_AlbumFields>> {
    static auto from(qcm::backend::Album album) -> Box<p_qcm::p_msg::p_entity::M_AlbumFields>;
};

template<>
struct Impl<convert::From<qcm::backend::Artist>, Box<p_qcm::p_msg::p_entity::M_ArtistFields>> {
    static auto from(qcm::backend::Artist artist) -> Box<p_qcm::p_msg::p_entity::M_ArtistFields>;
};

template<>
struct Impl<convert::From<qcm::backend::Mix>, Box<p_qcm::p_msg::p_entity::M_MixFields>> {
    static auto from(qcm::backend::Mix mix) -> Box<p_qcm::p_msg::p_entity::M_MixFields>;
};

template<>
struct Impl<convert::From<ref<qcm::backend::Song>>, Box<p_qcm::p_msg::p_model::M_Song>> {
    static auto from(ref<qcm::backend::Song> value) -> Box<p_qcm::p_msg::p_model::M_Song>;
};
template<>
struct Impl<convert::From<ref<qcm::backend::Album>>, Box<p_qcm::p_msg::p_model::M_Album>> {
    static auto from(ref<qcm::backend::Album> value) -> Box<p_qcm::p_msg::p_model::M_Album>;
};
template<>
struct Impl<convert::From<ref<qcm::backend::Artist>>, Box<p_qcm::p_msg::p_model::M_Artist>> {
    static auto from(ref<qcm::backend::Artist> value) -> Box<p_qcm::p_msg::p_model::M_Artist>;
};
template<>
struct Impl<convert::From<ref<qcm::backend::Mix>>, Box<p_qcm::p_msg::p_model::M_Mix>> {
    static auto from(ref<qcm::backend::Mix> value) -> Box<p_qcm::p_msg::p_model::M_Mix>;
};
template<>
struct Impl<convert::From<qcm::backend::Timestamp>, Box<p_google::p_protobuf::M_Timestamp>> {
    static auto from(qcm::backend::Timestamp value) -> Box<p_google::p_protobuf::M_Timestamp>;
};
template<>
struct Impl<convert::From<qcm::backend::DatabaseDateTime>, Box<p_google::p_protobuf::M_Timestamp>> {
    static auto from(qcm::backend::DatabaseDateTime value)
        -> Box<p_google::p_protobuf::M_Timestamp>;
};
template<>
struct Impl<convert::From<qcm::backend::Library>, Box<p_qcm::p_msg::p_model::M_Library>> {
    static auto from(qcm::backend::Library value) -> Box<p_qcm::p_msg::p_model::M_Library>;
};
template<>
struct Impl<convert::From<qcm::backend::HomeBlock>, Box<p_qcm::p_msg::p_model::M_HomeBlock>> {
    static auto from(qcm::backend::HomeBlock value) -> Box<p_qcm::p_msg::p_model::M_HomeBlock>;
};
template<>
struct Impl<convert::From<ref<qcm::backend::HomeBlock>>, Box<p_qcm::p_msg::p_model::M_HomeBlock>> {
    static auto from(ref<qcm::backend::HomeBlock> value) -> Box<p_qcm::p_msg::p_model::M_HomeBlock>;
};
} // namespace rstd
