module;
#include <rstd/enum.hpp>
export module qcm.backend:provider.resource;
export import :resource.prepared;
export import :model.image;
import :resource.identity;
using namespace rstd::prelude;

export namespace qcm::backend
{
[[pmacro::invoke(
    "rstd::Enum", [[derive(Clone)]] enum Resource {
                      Image { item_id: String, image_id: Option<String>, kind: ImageType },
                      Audio { item_id: String },
                  } members {
    public:
        auto kind() const -> resource_cache::ResourceKind;
        auto representation() const -> String;
    })]];

inline auto Resource::kind() const -> resource_cache::ResourceKind {
    RSTD_MATCH(*this) {
        RSTD_CASE(Image, item_id, image_id, kind) { return resource_cache::ResourceKind::Image; }
        RSTD_CASE(Audio, item_id) { return resource_cache::ResourceKind::Audio; }
    }
}
inline auto Resource::representation() const -> String {
    RSTD_MATCH(*this) {
        RSTD_CASE(Image, item_id, image_id, kind) {
            auto name = rstd::format("{}", kind);
            return resource_image_representation(item_id.as_str(), image_id, name.as_str());
        }
        RSTD_CASE(Audio, item_id) { return resource_audio_representation(item_id.as_str()); }
    }
}
} // namespace qcm::backend
