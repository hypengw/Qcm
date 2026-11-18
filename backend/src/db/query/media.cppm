export module qcm.backend:db.query.media;
export import :error;
import :model.image;
import sqlito;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct AudioResourceDescriptor {
    i64    provider_id;
    String native_id;
};
struct ImageResourceDescriptor {
    i64            provider_id;
    String         native_id;
    Option<String> image_native;
    ImageType      image_type;
};

auto audio_resource_descriptor(sqlito::Connection& db, i64 id)
    -> ProcessResult<Option<AudioResourceDescriptor>>;
auto image_resource_descriptor(sqlito::Connection& db, i64 id, ref<str> item_name,
                               ref<str> image_name)
    -> ProcessResult<Option<ImageResourceDescriptor>>;
} // namespace qcm::backend
