#include "Qcm/lyric.h"

#include <ctre.hpp>

#include "Qcm/type.h"
#include "core/log.h"

using namespace qcm;

namespace
{
static constexpr auto RE_LrcLine =
    ctll::fixed_string { "\\[([0-9]{2})[:]([0-9]{2})[.]([0-9]{2,3})\\](.*)" };
static constexpr auto RE_LrcTagLine = ctll::fixed_string { "\\[([^:]+)[:](.*?)\\]" };

QList<LrcLyricLine> parse_lrc(const QString& source) {
    std::map<qlonglong, LrcLyricLine> lrc_map;
    auto                              list = QStringView(source).split('\n');

    qlonglong last_milli { 0 };
    for (auto line_ : list) {
        // auto line = To<std::string>::from(line_.trimmed().toString());
        auto line = line_.trimmed().toUtf8();

        if (auto [whole, min, sec, milli, content] = ctre::match<RE_LrcLine>(line); whole) {
            qlonglong milli_total =
                (min.to_number() * 60 + sec.to_number()) * 1000 + milli.to_number();
            auto content_str = content.to_view();

            if (lrc_map.contains(milli_total)) {
                lrc_map.at(milli_total).content.append('\n').append(QString::fromUtf8(content_str));
            } else {
                lrc_map.insert({ milli_total,
                                 LrcLyricLine { .milliseconds = milli_total,
                                                .content      = QString::fromUtf8(content_str) } });
            }
            last_milli = milli_total;
        } else if (auto [whole, tag, tag_v] = ctre::match<RE_LrcTagLine>(line); whole) {
            // to do
        } else {
            // add this to previous
            if (lrc_map.contains(last_milli)) {
                lrc_map.at(last_milli).content.append('\n').append(QString::fromUtf8(line));
            }
        }
    }
    QList<LrcLyricLine> out;

    std::transform(lrc_map.begin(), lrc_map.end(), std::back_inserter(out), [](auto& el) {
        return el.second;
    });

    return out;
}

} // namespace

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
    resetModel(parse_lrc(m_source));
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
        if (it == begin) {
            m_cur_idx = -1;
        } else {
            m_cur_idx = std::distance(begin, it - 1);
        }
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
