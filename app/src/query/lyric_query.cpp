module;
#include "Qcm/query/lyric_query.moc.h"
module qcm;
import :query.lyric;

namespace qcm
{

LyricQuery::LyricQuery(QObject* parent): Query(parent) {
    connect_requet_reload(&LyricQuery::songIdChanged);
}
void LyricQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetSubtitleReq {};

    if (identity::valid(m_song_id)) {
        req.setSongId(m_song_id);

        auto self = QWatcher { this };
        spawn([self, backend, req] mutable -> task<void> {
            auto rsp = co_await backend->send(std::move(req));
            if (! co_await QAsyncResult::qexecutor()) co_return;
            if (! self) co_return;
            auto t = self->tdata();
            if (rsp) {
                msg::GetSubtitleRsp& el = *rsp;
                auto view = std::ranges::views::transform(el.subtitle().items(), [](auto& el) {
                    return LyricItem { .milliseconds = el.start(), .content = el.text() };
                });
                t->resetModel(view);
            } else {
                t->resetModel();
            }
            self->setStatus(Status::Finished);
            co_return;
        });
    } else {
        cancel();
        auto t = this->tdata();
        t->resetModel();
        setStatus(Status::Finished);
    }
}

auto LyricQuery::songId() const -> msg::id::SongId { return m_song_id; }
void LyricQuery::setSongId(msg::id::SongId id) {
    if (ycore::cmp_set(m_song_id, id)) {
        songIdChanged();
    }
}

} // namespace qcm

#include "Qcm/query/lyric_query.moc.cpp"
