export module qcm.backend:resource.identity;
export import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto resource_audio_representation(ref<str> native_id) -> String {
    auto result = rstd::format("audio:{}:", native_id.as_bytes().len().to_primitive());
    result.push_str(native_id);
    return result;
}
auto resource_image_representation(ref<str> native_id, const Option<String>& image_id,
                                   ref<str> kind) -> String {
    auto result = rstd::format("image:{}:", native_id.as_bytes().len().to_primitive());
    result.push_str(native_id);
    if (image_id.is_some()) {
        result.push_str(
            rstd::format(":1:{}:", image_id->as_str().as_bytes().len().to_primitive()).as_str());
        result.push_str(image_id->as_str());
    } else {
        result.push_str(":0"_str);
    }
    result.push_str(rstd::format(":{}:", kind.as_bytes().len().to_primitive()).as_str());
    result.push_str(kind);
    return result;
}

} // namespace qcm::backend
