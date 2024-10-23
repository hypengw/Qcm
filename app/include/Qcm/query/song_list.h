#pragma once

#pragma once

#include <QQmlEngine>

#include "Qcm/query/query.h"
#include "qcm_interface/async.inl"
#include "qcm_interface/model/song.h"

namespace qcm::query
{

template<typename T>
class SongListQuery : public Query<T> {
public:
    SongListQuery(QObject* parent = nullptr): Query<T>(parent) {}

public:
    void reload() override {}
};

} // namespace qcm::query
