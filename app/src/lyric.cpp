#include "Qcm/lyric.h"

#include <ctre.hpp>

#include "Qcm/type.h"
#include "core/log.h"

namespace
{
static constexpr auto RE_LrcLine =
    ctll::fixed_string { "\\[([0-9]{2})[:]([0-9]{2})[.]([0-9]{2,3})\\](.*)" };
static constexpr auto RE_LrcTagLine = ctll::fixed_string { "\\[([^:]+)[:].*?\\]" };

} // namespace

using namespace qcm;
LrcLyric::LrcLyric(QObject* parent)
    : meta_model::QGadgetListModel<LrcLyricLine>(parent), m_cur_idx(-1), m_position(0) {
    connect(this, &LrcLyric::sourceChanged, this, &LrcLyric::parseLrc);
    connect(this, &LrcLyric::positionChanged, this, &LrcLyric::refreshIndex);
}
LrcLyric::~LrcLyric() {}

qlonglong LrcLyric::position() const { return m_position; }
void      LrcLyric::setPosition(qlonglong v) {
    if (std::exchange(m_position, v) != v) {
        emit positionChanged();
    }
}

qlonglong LrcLyric::currentIndex() const { return m_cur_idx; }

QString LrcLyric::source() const { return m_source; }
void    LrcLyric::setSource(QString v) {
    if (std::exchange(m_source, v) != v) {
        emit sourceChanged();
    }
}

void LrcLyric::parseLrc() {
    QList<LrcLyricLine> out;
    auto                list = QStringView(m_source).split('\n');
    for (auto line_ : list) {
        auto line = To<std::string>::from(line_.trimmed().toString());

        if (auto [whole, min, sec, milli, content] = ctre::match<RE_LrcLine>(line); whole) {
            i64  milli_total = (min.to_number() * 60 + sec.to_number()) * 1000 + milli.to_number();
            auto content_str = content.to_view();

            out.emplace_back(LrcLyricLine { .milliseconds = milli_total,
                                            .content      = QString::fromUtf8(content_str) });
        }
    }

    std::stable_sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return a.milliseconds < b.milliseconds;
    });

    resetModel(out);
    refreshIndex();
}

void LrcLyric::refreshIndex() {
    auto old   = m_cur_idx;
    auto begin = items().begin();
    auto end   = items().end();

    if (m_cur_idx < 0 || m_cur_idx >= rowCount()) {
        auto it = std::upper_bound(begin, end, m_position, [](const auto& val, const auto& el) {
            return val < el.milliseconds;
        });
        if (it != begin) it--;
        m_cur_idx = std::distance(begin, it);
    } else {
        auto it = begin + m_cur_idx;
        while (it != end && (*it).milliseconds < m_position) {
            it++;
        }

        while (it != begin && (*it).milliseconds > m_position) {
            it--;
        }

        m_cur_idx = std::distance(begin, it);
    }
    if (old != m_cur_idx) {
        emit currentIndexChanged();
    }
}
