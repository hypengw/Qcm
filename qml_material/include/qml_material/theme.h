#pragma once

#include <functional>

#include <QObject>
#include <QQmlEngine>
#include <QQuickAttachedPropertyPropagator>
#include <QColor>

#define ATTACH_PROPERTY(_type_, _name_)                                                 \
private:                                                                                \
    Q_PROPERTY(_type_ _name_ READ _name_ WRITE set_##_name_ RESET reset_##_name_ NOTIFY \
                   _name_##Changed FINAL)                                               \
public:                                                                                 \
    _type_              _name_() const;                                                 \
    void                set_##_name_(const _type_&);                                    \
    void                reset_##_name_();                                               \
    AttachProp<_type_>& get_##_name_();                                                 \
                                                                                        \
private:                                                                                \
    AttachProp<_type_> m_##_name_;

namespace qml_material
{

class Theme : public QQuickAttachedPropertyPropagator {
    Q_OBJECT

    QML_NAMED_ELEMENT(MatProp)
    QML_UNCREATABLE("")
    QML_ATTACHED(Theme)

public:
    using Self = Theme;

    using SigFunc = void (Theme::*)();
    template<typename V>
    struct AttachProp {
        V       value;
        bool    explicited;
        SigFunc sigfunc;

        AttachProp(SigFunc s): value(), explicited(false), sigfunc(s) {}
    };

    ATTACH_PROPERTY(QColor, textColor)
    ATTACH_PROPERTY(QColor, backgroundColor)
    ATTACH_PROPERTY(QColor, stateLayerColor)
    ATTACH_PROPERTY(int, elevation)

public:
    Theme(QObject* parent);
    ~Theme();

    static Theme* qmlAttachedProperties(QObject* object);

Q_SIGNALS:
    void textColorChanged();
    void backgroundColorChanged();
    void elevationChanged();
    void stateLayerColorChanged();

protected:
    void attachedParentChange(QQuickAttachedPropertyPropagator* newParent,
                              QQuickAttachedPropertyPropagator* oldParent) override;
};
} // namespace qml_material

#undef ATTACH_PROPERTY