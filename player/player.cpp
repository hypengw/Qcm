#include <cubeb/cubeb.h>
#include <cassert>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/as_tuple.hpp>

#include "player/notify.h"
#include "player/player.h"
#include "stream_reader.h"
#include "player/player_p.h"

using namespace player;

Player::Player(std::string_view name, Notifier notifier, executor_type exc)
    : m_d(make_up<Private>(name, notifier, exc)) {}
Player::~Player() {}

Player::Private::Private(std::string_view name, Notifier notifier, executor_type exc)
    : m_notifier(notifier),
      m_action_channel(make_rc<action_channel_type>(exc, 64)),
      m_action_id(0),
      m_end(false),
      m_reader(make_rc<StreamReader>(notifier)),
      m_dec(make_up<Decoder>()),
      m_dev(make_up<Device>(make_rc<DeviceContext>(name), nullptr, 2, 44100, notifier)),
      m_ctx(make_rc<Context>()) {
    auto channel = m_action_channel;
    asio::co_spawn(
        asio::strand<action_channel_type::executor_type>(channel->get_executor()),
        [this, channel]() -> asio::awaitable<void> {
            while (! m_end) {
                auto [ec, info] =
                    co_await channel->async_receive(asio::as_tuple(asio::use_awaitable));
                if (! ec && ! m_end) {
                    this->m_notifier.try_send(notify::busy { true });
                    u32 id = std::visit(overloaded {
                                            [this](action::play a) {
                                                this->play();
                                                return a.id;
                                            },
                                            [this](action::pause a) {
                                                this->pause();
                                                return a.id;
                                            },
                                            [this](action::stop a) {
                                                this->stop();
                                                return a.id;
                                            },
                                            [this](action::seek a) {
                                                this->seek(a.value);
                                                return a.id;
                                            },
                                            [this](action::source a) {
                                                this->set_source(a.value);
                                                return a.id;
                                            },
                                        },
                                        info);
                    if (m_action_id == id + 1) {
                        this->m_notifier.try_send(notify::busy { false });
                    }
                }
            }
            co_return;
        },
        asio::detached);
}

Player::Private::~Private() {
    m_end = true;
    m_action_channel->cancel();
}

void Player::Private::set_source(std::string_view v) {
    stop();
    m_ctx->clear();

    if (v.empty()) return;

    m_ctx->set_aborted(false);

    m_dev->set_output(m_ctx->audio_frame_queue);
    m_reader->start(v, m_ctx->audio_pkt_queue);
    m_dec->start(m_reader, m_ctx->audio_pkt_queue, m_ctx->audio_frame_queue);

    m_dev->start();
    play();
}

void Player::Private::play() { m_dev->set_pause(false); }
void Player::Private::pause() { m_dev->set_pause(true); }
void Player::Private::stop() {
    m_dev->mark_dirty();

    m_ctx->set_aborted(true);
    m_reader->stop();
    m_dec->stop();

    m_notifier.send(notify::position { 0 });
    m_notifier.send(notify::playstate { PlayState::Stopped }).wait();
}

void Player::Private::seek(i32 p) {
    m_dev->mark_dirty();
    m_reader->seek(p);
    m_ctx->audio_pkt_queue->wake_one_pusher();
}

void Player::set_source(std::string_view v) {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {},
                                  action::source { std::string(v), d->m_action_id++ });
}

void Player::play() {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::play { d->m_action_id++ });
}
void Player::pause() {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::pause { d->m_action_id++ });
}
void Player::stop() {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::stop { d->m_action_id++ });
}
void Player::seek(i32 p) {
    C_D(Player);
    d->m_action_channel->try_send(asio::error_code {}, action::seek { p, d->m_action_id++ });
}

auto Player::volume() const -> float {
    C_D(const Player);
    return d->m_dev->volume();
}
void Player::set_volume(float val) {
    C_D(Player);
    d->m_dev->set_volume(val);
}
auto Player::fade_time() const -> u32 {
    C_D(const Player);
    return d->m_dev->fade_duration();
}
void Player::set_fade_time(u32 val) {
    C_D(Player);
    d->m_dev->set_fade_duration(val);
}

namespace
{

auto getMimeType(AVCodecID codec_id) -> std::string_view {
    // Mapping AVCodecID to MIME types
    static const std::unordered_map<AVCodecID, std::string> codecToMimeMap = {
        { AV_CODEC_ID_4XM, "audio/x-adpcm" },
        { AV_CODEC_ID_AAC, "audio/aac" },
        { AV_CODEC_ID_AC3, "audio/x-ac3" },
        { AV_CODEC_ID_AAC, "audio/aac" },
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
        { AV_CODEC_ID_WEBVTT, "text/vtt" }
    };

    auto it = codecToMimeMap.find(codec_id);
    if (it != codecToMimeMap.end()) {
        return it->second;
    } else {
        return "unknown/unknown";
    }
}

} // namespace

auto player::get_metadata(const std::filesystem::path& path) -> Metadata {
    Metadata            meta;
    FFmpegFormatContext ctx;
    FFmpegError         err;
    do {
        err = ctx.open_input(path.string().c_str());

        if (err) break;

        err = ctx.find_stream_info(NULL);
        if (err) break;

        AVDictionaryEntry* tag { NULL };
        while ((tag = av_dict_get(ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            meta.tags.insert({ tag->key, tag->value });
        }

        for (u32 i = 0; i < ctx->nb_streams; i++) {
            Metadata::Stream smeta;
            AVStream*        stream = ctx->streams[i];

            if (stream->codecpar->bit_rate > 0) {
                smeta.bitrate = stream->codecpar->bit_rate;
            }
            smeta.mime = getMimeType(stream->codecpar->codec_id);
            meta.streams.push_back(smeta);
        }
        return meta;
    } while (false);
    return {};
}