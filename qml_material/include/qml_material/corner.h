#pragma once
#include <QQmlEngine>
#include <QVector4D>

namespace qml_material
{
class CornersGroup {
    Q_GADGET
    QML_VALUE_TYPE(corner_t)

    Q_PROPERTY(qreal topLeft READ topLeft WRITE setTopLeft FINAL)
    Q_PROPERTY(qreal topRight READ topRight WRITE setTopRight FINAL)
    Q_PROPERTY(qreal bottomLeft READ bottomLeft WRITE setBottomLeft FINAL)
    Q_PROPERTY(qreal bottomRight READ bottomRight WRITE setBottomRight FINAL)

public:
    CornersGroup();
    CornersGroup(qreal);
    CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft, qreal topLeft);
    ~CornersGroup();

    Q_INVOKABLE CornersGroup& operator=(qreal) { return *this; }

    qreal topLeft() const;
    void  setTopLeft(qreal newTopLeft);

    qreal topRight() const;
    void  setTopRight(qreal newTopRight);

    qreal bottomLeft() const;
    void  setBottomLeft(qreal newBottomLeft);

    qreal bottomRight() const;
    void  setBottomRight(qreal newBottomRight);

    Q_INVOKABLE QVector4D toVector4D() const;

private:
    float m_bottomRight;
    float m_topRight;
    float m_bottomLeft;
    float m_topLeft;
};

} // namespace qml_material