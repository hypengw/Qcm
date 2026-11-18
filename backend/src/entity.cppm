export module qcm.backend:entity;
export import :protocol.types;
export import rstd;
export import sqlito;
import :db.sync.run;
import :db.error.query;

export namespace qcm::backend
{
using EntityBatch  = p_qcm::p_msg::p_entity::M_EntityBatch;
using SongUpdate   = p_qcm::p_msg::p_entity::M_SongUpdate;
using AlbumUpdate  = p_qcm::p_msg::p_entity::M_AlbumUpdate;
using ArtistUpdate = p_qcm::p_msg::p_entity::M_ArtistUpdate;
using MixUpdate    = p_qcm::p_msg::p_entity::M_MixUpdate;

auto qcm_entity_batch(sqlito::Transaction& db, const ProjectionSnapshot& snapshot,
                      const p_qcm::p_msg::M_QcmMessage& response,
                      const p_qcm::p_msg::M_QcmMessage* request = nullptr)
    -> QueryResult<rstd::prelude::Box<EntityBatch>>;
} // namespace qcm::backend
