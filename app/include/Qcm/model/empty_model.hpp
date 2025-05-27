#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "Qcm/backend_msg.hpp"

namespace qcm::model
{
class EmptyModel : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::model::Song song READ song CONSTANT FINAL)
public:
    EmptyModel(QObject* parent = nullptr): QObject(parent) {}
    ~EmptyModel() {}

    auto song() const -> const model::Song& { return m_song; }

private:
    model::Song m_song;
};
} // namespace qcm::model