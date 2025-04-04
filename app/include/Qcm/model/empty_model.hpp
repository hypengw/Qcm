#pragma once

#include <QQmlEngine>
#include "core/core.h"

namespace qcm::model
{
class EmptyModel : public QObject {
    Q_OBJECT
    // Q_PROPERTY(qcm::query::Song song READ song CONSTANT FINAL)
public:
    EmptyModel(QObject* parent = nullptr): QObject(parent) { 
        // m_song.id.set_id("empty"); 
    }
    ~EmptyModel() {}

    // auto song() const { return m_song; }

private:
    // query::Song m_song;
};
} // namespace qcm::model