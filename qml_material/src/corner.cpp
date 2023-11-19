#include "qml_material/corner.h"

using namespace qml_material;

CornersGroup::CornersGroup(): CornersGroup(0) {}
CornersGroup::CornersGroup(qreal r): CornersGroup(r, r, r, r) {}
CornersGroup::CornersGroup(qreal bottomRight, qreal topRight, qreal bottomLeft, qreal topLeft)
    : m_bottomRight(bottomRight),
      m_topRight(topRight),
      m_bottomLeft(bottomLeft),
      m_topLeft(topLeft) {}
CornersGroup::~CornersGroup() {}

qreal CornersGroup::topLeft() const { return m_topLeft; }

void CornersGroup::setTopLeft(qreal newTopLeft) {
    if (newTopLeft == m_topLeft) {
        return;
    }

    m_topLeft = newTopLeft;
}

qreal CornersGroup::topRight() const { return m_topRight; }

void CornersGroup::setTopRight(qreal newTopRight) {
    if (newTopRight == m_topRight) {
        return;
    }

    m_topRight = newTopRight;
}

qreal CornersGroup::bottomLeft() const { return m_bottomLeft; }

void CornersGroup::setBottomLeft(qreal newBottomLeft) {
    if (newBottomLeft == m_bottomLeft) {
        return;
    }

    m_bottomLeft = newBottomLeft;
}

qreal CornersGroup::bottomRight() const { return m_bottomRight; }

void CornersGroup::setBottomRight(qreal newBottomRight) {
    if (newBottomRight == m_bottomRight) {
        return;
    }

    m_bottomRight = newBottomRight;
}

QVector4D CornersGroup::toVector4D() const {
    return QVector4D { m_bottomRight, m_topRight, m_bottomLeft, m_topLeft };
}
