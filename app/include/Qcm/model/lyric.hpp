#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "meta_model/qgadget_list_model.hpp"

namespace qcm
{

class LyricItem {
    Q_GADGET
public:
    Q_PROPERTY(qlonglong milliseconds MEMBER milliseconds FINAL)
    Q_PROPERTY(QString content MEMBER content FINAL)

    qlonglong milliseconds;
    QString   content;
};

class LyricModel : public meta_model::QGadgetListModel<LyricItem> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qlonglong currentIndex READ currentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(qlonglong position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged FINAL)
public:
    LyricModel(QObject* = nullptr);
    ~LyricModel();

    QString source() const;
    void    setSource(QString);

    qlonglong position() const;
    void      setPosition(qlonglong);

    qlonglong currentIndex() const;

    Q_SIGNAL void currentIndexChanged(qlonglong);
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void sourceChanged();

    Q_SLOT void setCurrentIndex(qlonglong);

private:
    Q_SLOT void parseLrc();
    Q_SLOT void refreshIndex();

private:
    qlonglong m_cur_idx;
    qlonglong m_position;
    QString   m_source;
};

} // namespace qcm
