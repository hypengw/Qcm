module;
#include "Qcm/model/lyric.moc.h"
#include <algorithm>
#include "core/log.h"

module qcm;
import :model.lyric;
import qextra;
import :global;

using namespace qcm;

LyricModel::LyricModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent), m_cur_idx(-1), m_position(0) {
    connect(this, &LyricModel::positionChanged, this, &LyricModel::refreshIndex);
    connect(this, &LyricModel::modelReset, this, &LyricModel::refreshIndex);
    connect(this, &LyricModel::rowsInserted, this, &LyricModel::refreshIndex);
    connect(this, &LyricModel::rowsRemoved, this, &LyricModel::refreshIndex);
    connect(this, &LyricModel::dataChanged, this, &LyricModel::refreshIndex);
}
LyricModel::~LyricModel() {}

qlonglong LyricModel::position() const { return m_position; }
void      LyricModel::setPosition(qlonglong v) {
    if (std::exchange(m_position, v) != v) {
        emit positionChanged();
    }
}

qlonglong LyricModel::currentIndex() const { return m_cur_idx; }

void LyricModel::setCurrentIndex(qlonglong v) {
    if (std::exchange(m_cur_idx, v) != v) {
        currentIndexChanged(v);
    }
}

QString LyricModel::source() const { return m_source; }
void    LyricModel::setSource(QString v) {
    if (std::exchange(m_source, v) != v) {
        emit sourceChanged();
    }
}

void LyricModel::refreshIndex() {
    const auto next = std::upper_bound(begin(),
                                       end(),
                                       std::max<qlonglong>(m_position, 0),
                                       [](qlonglong position, const LyricItem& item) {
                                           return position < item.milliseconds;
                                       });
    setCurrentIndex(std::distance(begin(), next) - 1);
}

#include "Qcm/model/lyric.moc.cpp"
