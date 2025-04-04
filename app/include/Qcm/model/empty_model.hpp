#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "Qcm/backend_msg.hpp"

namespace qcm::model
{
class EmptyModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Song song READ song CONSTANT FINAL)
public:
    EmptyModel(QObject* parent = nullptr): QObject(parent) {}
    ~EmptyModel() {}

    auto song() const { return m_song; }

private:
    model::Song m_song;
};
} // namespace qcm::model