#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <utility>

extern "C" {
#include <libavformat/avformat.h>
}

#include "player/player.h"

namespace
{

auto mime_type(AVCodecID codec_id) -> std::string_view {
    static const std::unordered_map<AVCodecID, std::string_view> codec_to_mime {
        { AV_CODEC_ID_4XM, "audio/x-adpcm" },
        { AV_CODEC_ID_AAC, "audio/aac" },
        { AV_CODEC_ID_AC3, "audio/x-ac3" },
        { AV_CODEC_ID_AMR_NB, "audio/amr" },
        { AV_CODEC_ID_AMR_WB, "audio/amr" },
        { AV_CODEC_ID_APNG, "image/png" },
        { AV_CODEC_ID_ASS, "text/x-ass" },
        { AV_CODEC_ID_DTS, "audio/x-dca" },
        { AV_CODEC_ID_DVD_NAV, "video/mpeg" },
        { AV_CODEC_ID_EAC3, "audio/x-eac3" },
        { AV_CODEC_ID_FLAC, "audio/x-flac" },
        { AV_CODEC_ID_FLV1, "video/x-flv" },
        { AV_CODEC_ID_GIF, "image/gif" },
        { AV_CODEC_ID_GSM, "audio/x-gsm" },
        { AV_CODEC_ID_H261, "video/x-h261" },
        { AV_CODEC_ID_H263, "video/x-h263" },
        { AV_CODEC_ID_ILBC, "audio/iLBC" },
        { AV_CODEC_ID_JACOSUB, "text/x-jacosub" },
        { AV_CODEC_ID_JPEG2000, "image/jpeg" },
        { AV_CODEC_ID_AAC_LATM, "audio/MP4A-LATM" },
        { AV_CODEC_ID_MJPEG, "video/x-mjpeg" },
        { AV_CODEC_ID_MPEG1VIDEO, "video/mpeg" },
        { AV_CODEC_ID_MP3, "audio/mpeg" },
        { AV_CODEC_ID_OPUS, "audio/ogg" },
        { AV_CODEC_ID_SRT, "application/x-subrip" },
        { AV_CODEC_ID_ADPCM_SWF, "application/x-shockwave-flash" },
        { AV_CODEC_ID_WEBP, "image/webp" },
        { AV_CODEC_ID_WEBVTT, "text/vtt" },
    };

    auto iter = codec_to_mime.find(codec_id);
    return iter == codec_to_mime.end() ? "unknown/unknown" : iter->second;
}

} // namespace

auto player::get_metadata(const std::filesystem::path& path) -> Metadata {
    struct Context {
        ~Context() {
            if (value != nullptr) avformat_close_input(&value);
        }
        AVFormatContext* value { nullptr };
    } context;

    if (avformat_open_input(&context.value, path.string().c_str(), nullptr, nullptr) < 0) return {};
    if (avformat_find_stream_info(context.value, nullptr) < 0) return {};

    auto metadata = Metadata {};
    auto tag       = static_cast<AVDictionaryEntry*>(nullptr);
    while ((tag = av_dict_get(context.value->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        metadata.tags.insert_or_assign(tag->key, tag->value);
    }

    for (unsigned int index = 0; index < context.value->nb_streams; ++index) {
        const auto* stream = context.value->streams[index];
        auto        item   = Metadata::Stream {};
        if (stream->codecpar->bit_rate > 0) item.bitrate = stream->codecpar->bit_rate;
        item.mime = mime_type(stream->codecpar->codec_id);
        metadata.streams.push_back(std::move(item));
    }
    return metadata;
}
