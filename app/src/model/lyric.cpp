#include "Qcm/model/lyric.hpp"

#include <limits>

#include "core/core.h"
#include "core/log.h"

import qcm.helper;

using namespace qcm;

LyricModel::LyricModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent), m_cur_idx(-1), m_position(0) {
    // connect(this, &Lyric::sourceChanged, this, &Lyric::parseLrc);
    connect(this, &LyricModel::positionChanged, this, &LyricModel::refreshIndex);
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
    auto old   = m_cur_idx;
    auto begin = this->begin();
    auto end   = this->end();

    if (begin < end) {
        auto it = begin + m_cur_idx;

        auto pos = m_position < 0 ? 0 : m_position;

        auto milli = [begin, end](auto it) -> qlonglong {
            if (it < begin)
                return -1;
            else if (it >= end)
                return std::numeric_limits<qlonglong>::max();
            else
                return it->milliseconds;
        };

        // check left
        while (milli(it) > pos) {
            it--;
        }

        // check right
        while (milli(it + 1) <= pos) {
            it++;
        }
        m_cur_idx = std::distance(begin, it);
    } else
        m_cur_idx = -1;

    if (old != m_cur_idx) {
        emit currentIndexChanged(m_cur_idx);
    }
}

#include <Qcm/model/moc_lyric.cpp>