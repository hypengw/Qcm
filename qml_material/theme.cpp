#include "qml_material/theme.h"

using namespace qml_material;

namespace
{
static QColor global_textColor {};
static QColor global_supportTextColor {};
static QColor global_backgroundColor {};
static int    global_elevation { 0 };
static QColor global_stateLayerColor {};

using Propagator = QQuickAttachedPropertyPropagator;

template<typename F, typename T>
concept get_prop_cp = requires(F f, Theme* t) {
    { std::invoke(f, t) } -> std::same_as<Theme::AttachProp<T>&>;
};

template<typename F>
void propagate(Theme* self, F&& f) {
    const auto styles = self->attachedChildren();
    for (auto* child : styles) {
        Theme* obj = qobject_cast<Theme*>(child);
        if (obj) {
            f(obj);
        }
    }
}

template<typename F, typename T>
    requires get_prop_cp<F, T>
void inherit_attach_prop(Theme* self, F&& get_prop, const T& v) {
    auto& p = std::invoke(get_prop, self);
    if (p.explicited || p.value == v) return;
    p.value = v;
    propagate(self, [&v, &get_prop](Theme* child) {
        inherit_attach_prop(child, get_prop, v);
    });
    std::invoke(p.sigfunc, self);
}

template<typename F, typename T>
    requires get_prop_cp<F, T>
void set_prop(Theme* self, const T& v, F&& get_prop) {
    auto& p      = std::invoke(std::forward<F>(get_prop), self);
    p.explicited = true;
    if (std::exchange(p.value, v) != v) {
        propagate(self, [&v, &get_prop](Theme* child) {
            inherit_attach_prop(child, get_prop, v);
        });
        std::invoke(p.sigfunc, self);
    }
}

template<typename F, typename T>
    requires get_prop_cp<F, T>
void reset_prop(Theme* self, F&& get_prop, const T& init_v) {
    auto& p = std::invoke(std::forward<F>(get_prop), self);
    if (! p.explicited) return;
    p.explicited = false;
    inherit_attach_prop(self, get_prop, init_v);
}

} // namespace

Theme::Theme(QObject* parent)
    : QQuickAttachedPropertyPropagator(parent),
      m_textColor(&Theme::textColorChanged),
      m_supportTextColor(&Theme::supportTextColorChanged),
      m_backgroundColor(&Theme::backgroundColorChanged),
      m_stateLayerColor(&Theme::stateLayerColorChanged),
      m_elevation(&Theme::elevationChanged) {
    QQuickAttachedPropertyPropagator::initialize();
}
Theme::~Theme() {}

Theme* Theme::qmlAttachedProperties(QObject* object) { return new Theme(object); }

#define IMPL_ATTACH_PROP(_type_, _name_, _prop_)                                       \
    Theme::AttachProp<_type_>& Theme::get_##_name_() { return _prop_; }                \
    _type_                     Theme::_name_() const { return _prop_.value; }          \
    void                       Theme::set_##_name_(const _type_& v) {                  \
        set_prop(this, v, [](Self* o) -> AttachProp<_type_>& {   \
            return o->_prop_;                                    \
        });                                                      \
    }                                                                                  \
    void Theme::reset_##_name_() {                                                     \
        Self* obj = qobject_cast<Self*>(attachedParent());                             \
        reset_prop(this, &Theme::get_##_name_, obj ? obj->_name_() : global_##_name_); \
    }

IMPL_ATTACH_PROP(QColor, textColor, m_textColor)
IMPL_ATTACH_PROP(QColor, supportTextColor, m_supportTextColor)
IMPL_ATTACH_PROP(QColor, backgroundColor, m_backgroundColor)
IMPL_ATTACH_PROP(int, elevation, m_elevation)
IMPL_ATTACH_PROP(QColor, stateLayerColor, m_stateLayerColor)

void Theme::attachedParentChange(QQuickAttachedPropertyPropagator* newParent,
                                 QQuickAttachedPropertyPropagator* oldParent) {
    Q_UNUSED(oldParent);
    Theme* attachedParentStyle = qobject_cast<Theme*>(newParent);
    if (attachedParentStyle) {
        #define X(_name_) inherit_attach_prop(this, &Theme::get_##_name_, attachedParentStyle->_name_())
        X(textColor);
        X(supportTextColor);
        X(backgroundColor);
        X(stateLayerColor);
        X(elevation);
        #undef X
    }
}