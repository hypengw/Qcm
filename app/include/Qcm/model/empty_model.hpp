#pragma once

#include <QQmlEngine>
#include "qcm_interface/export.h"

namespace qcm::model
{
class QCM_INTERFACE_API EmptyModel : public QObject {
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