#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtQml/QQmlEngine>
#include <QtGui/QColor>
#include <QtQuickControls2/QQuickAttachedPropertyPropagator>

#include "qml_material/color.h"

#define ATTACH_PROPERTY(_type_, _name_)                                                 \
private:                                                                                \
    Q_PROPERTY(_type_ _name_ READ _name_ WRITE set_##_name_ RESET reset_##_name_ NOTIFY \
                   _name_##Changed FINAL)                                               \
public:                                                                                 \
    _type_              _name_() const;                                                 \
    void                set_##_name_(_type_);                                           \
    void                reset_##_name_();                                               \
    AttachProp<_type_>& get_##_name_();                                                 \
    Q_SIGNAL void       _name_##Changed();                                              \
                                                                                        \
private:                                                                                \
    AttachProp<_type_> m_##_name_ { &Self::_name_##Changed };

namespace qml_material
{

class Theme : public QQuickAttachedPropertyPropagator {
    Q_OBJECT

    QML_NAMED_ELEMENT(MatProp)
    QML_UNCREATABLE("")
    QML_ATTACHED(Theme)

public:
    using Self = Theme;

    template<typename V>
    struct AttachProp {
        using SigFunc   = void (Theme::*)();
        using ReadFunc  = V (Theme::*const)();
        using WriteFunc = void (Theme::*)(V);
        using GetFunc   = AttachProp<V>& (Theme::*const)();

        std::optional<V> value;
        bool             explicited;
        SigFunc          sig_func;

        AttachProp(SigFunc s): value(), explicited(false), sig_func(s) {}
    };

    ATTACH_PROPERTY(QColor, textColor)
    ATTACH_PROPERTY(QColor, supportTextColor)
    ATTACH_PROPERTY(QColor, backgroundColor)
    ATTACH_PROPERTY(QColor, stateLayerColor)
    ATTACH_PROPERTY(int, elevation)
    ATTACH_PROPERTY(MdColorMgr*, color)

public:
    Theme(QObject* parent);
    ~Theme();

    static Theme* qmlAttachedProperties(QObject* object);

protected:
    void attachedParentChange(QQuickAttachedPropertyPropagator* newParent,
                              QQuickAttachedPropertyPropagator* oldParent) override;
};
} // namespace qml_material

#undef ATTACH_PROPERTY