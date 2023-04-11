#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "meta_model/qgadgetlistmodel.h"

namespace qcm
{

class LrcLyricLine {
    Q_GADGET
public:
    Q_PROPERTY(qlonglong milliseconds MEMBER milliseconds)
    Q_PROPERTY(QString content MEMBER content)

    qlonglong milliseconds;
    QString   content;
};

class LrcLyric : public meta_model::QGadgetListModel<LrcLyricLine> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qlonglong currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(qlonglong position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
public:
    LrcLyric(QObject* = nullptr);
    ~LrcLyric();

    QString source() const;
    void    setSource(QString);

    qlonglong position() const;
    void setPosition(qlonglong);

    qlonglong currentIndex() const;

signals:
    void currentIndexChanged();
    void positionChanged();
    void sourceChanged();

private slots:
    void parseLrc();
    void refreshIndex();
    void setCurrentIndex(qlonglong);

private:
    qlonglong m_cur_idx;
    qlonglong m_position;
    QString   m_source;
};

} // namespace qcm
