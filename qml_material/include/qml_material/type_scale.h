#pragma once

#include <QQmlEngine>
#include <QFont>
#include "core/core.h"

namespace qml_material
{

struct TypeScaleItem {
    Q_GADGET
    QML_ELEMENT
    QML_VALUE_TYPE(t_typescale)

    Q_PROPERTY(i32 size MEMBER size)
    Q_PROPERTY(i32 line_height MEMBER line_height)
    Q_PROPERTY(QFont::Weight weight MEMBER weight)
    Q_PROPERTY(QFont::Weight weight_prominent MEMBER weight_prominent)
    Q_PROPERTY(qreal tracking MEMBER tracking)
public:
    Q_INVOKABLE TypeScaleItem fresh() const { return *this; }

    i32           size;
    i32           line_height;
    QFont::Weight weight;
    QFont::Weight weight_prominent;
    qreal         tracking;
};

class TypeScale : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;

#define X(NAME, ...)                                                 \
    Q_PROPERTY(TypeScaleItem NAME READ NAME NOTIFY typescaleChanged) \
public:                                                              \
    TypeScaleItem NAME() const { return m_##NAME; }           \
                                                                     \
private:                                                             \
    TypeScaleItem m_##NAME { __VA_ARGS__ };
    // clang-format off
    X(display_large  , 57, 64, QFont::Normal, QFont::Normal, -0.25)
    X(display_medium , 45, 52, QFont::Normal, QFont::Normal, 0.0  )
    X(display_small  , 36, 44, QFont::Normal, QFont::Normal, 0.0  )
    X(headline_large , 32, 40, QFont::Medium, QFont::Medium, 0.0  )
    X(headline_medium, 28, 36, QFont::Medium, QFont::Medium, 0.0  )
    X(headline_small , 24, 32, QFont::Medium, QFont::Medium, 0.0  )
    X(title_large    , 22, 28, QFont::Normal, QFont::Normal, 0.0  )
    X(title_medium   , 16, 24, QFont::Medium, QFont::Medium, 0.15 )
    X(title_small    , 14, 20, QFont::Medium, QFont::Medium, 0.1  )
    X(body_large     , 16, 24, QFont::Normal, QFont::Normal, 0.5  )
    X(body_medium    , 14, 20, QFont::Normal, QFont::Normal, 0.25 )
    X(body_small     , 12, 16, QFont::Normal, QFont::Normal, 0.4  )
    X(label_large    , 14, 20, QFont::Medium, QFont::Bold  , 0.1  )
    X(label_medium   , 12, 16, QFont::Medium, QFont::Bold  , 0.5  )
    X(label_small    , 11, 16, QFont::Medium, QFont::Bold  , 0.5  )
    // clang-format on

#undef X

Q_SIGNALS:
    void typescaleChanged();
};

} // namespace qml_material