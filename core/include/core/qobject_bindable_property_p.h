#pragma once

#include <QtCore/QPropertyData>

namespace qcm
{
template<typename Class, typename T, auto Signal = nullptr>
class ObjectBindableProperty : public QPropertyData<T> {
    using ThisType                  = ObjectBindableProperty<Class, T, Signal>;
    static bool constexpr HasSignal = ! std::is_same_v<decltype(Signal), std::nullptr_t>;
    using SignalTakesValue          = std::is_invocable<decltype(Signal), Class, T>;

    Class*       owner() { return m_owner; }
    const Class* owner() const { return m_owner; }

    static void signalCallBack(QUntypedPropertyData* o) {
        ObjectBindableProperty* that = static_cast<ObjectBindableProperty*>(o);
        if constexpr (HasSignal) {
            if constexpr (SignalTakesValue::value)
                (that->owner()->*Signal)(that->valueBypassingBindings());
            else
                (that->owner()->*Signal)();
        }
    }

public:
    using value_type            = typename QPropertyData<T>::value_type;
    using parameter_type        = typename QPropertyData<T>::parameter_type;
    using rvalue_ref            = typename QPropertyData<T>::rvalue_ref;
    using arrow_operator_result = typename QPropertyData<T>::arrow_operator_result;

    ObjectBindableProperty(Class* p): m_owner(p) {}
    explicit ObjectBindableProperty(const T& initialValue, Class* p): QPropertyData<T>(initialValue), m_owner(p) {}
    explicit ObjectBindableProperty(T&& initialValue, Class* p): QPropertyData<T>(std::move(initialValue)), m_owner(p) {}
    // explicit ObjectBindableProperty(const QPropertyBinding<T>& binding): ObjectBindableProperty()
    // {
    //     setBinding(binding);
    // }
    // template<typename Functor>
    // explicit ObjectBindableProperty(
    //     Functor&&                             f,
    //     const QPropertyBindingSourceLocation& location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
    //     typename std::enable_if_t<std::is_invocable_r_v<T, Functor&>>* = nullptr)
    //     : ObjectBindableProperty(QPropertyBinding<T>(std::forward<Functor>(f), location)) {}

    parameter_type value() const {
        qGetBindingStorage(owner())->registerDependency(this);
        return this->val;
    }

    arrow_operator_result operator->() const {
        if constexpr (QTypeTraits::is_dereferenceable_v<T>) {
            return value();
        } else if constexpr (std::is_pointer_v<T>) {
            value();
            return this->val;
        } else {
            return;
        }
    }

    parameter_type operator*() const { return value(); }

    operator parameter_type() const { return value(); }

    void setValue(parameter_type t) {
        auto* bd = qGetBindingStorage(owner())->bindingData(this);
        if (bd) bd->removeBinding();
        if (this->val == t) return;
        this->val = t;
        notify(bd);
    }

    void notify() {
        auto* bd = qGetBindingStorage(owner())->bindingData(this);
        notify(bd);
    }

    void setValue(rvalue_ref t) {
        auto* bd = qGetBindingStorage(owner())->bindingData(this);
        if (bd) bd->removeBinding();
        if (this->val == t) return;
        this->val = std::move(t);
        notify(bd);
    }

    ObjectBindableProperty& operator=(rvalue_ref newValue) {
        setValue(std::move(newValue));
        return *this;
    }

    ObjectBindableProperty& operator=(parameter_type newValue) {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T>& newBinding) {
        QtPrivate::QPropertyBindingData* bd = qGetBindingStorage(owner())->bindingData(this, true);
        QUntypedPropertyBinding          oldBinding(
            bd->setBinding(newBinding, this, HasSignal ? &signalCallBack : nullptr));
        return static_cast<QPropertyBinding<T>&>(oldBinding);
    }

    bool setBinding(const QUntypedPropertyBinding& newBinding) {
        if (! newBinding.isNull() && newBinding.valueMetaType().id() != qMetaTypeId<T>())
            return false;
        setBinding(static_cast<const QPropertyBinding<T>&>(newBinding));
        return true;
    }

    template<typename Functor>
    QPropertyBinding<T> setBinding(
        Functor&&                             f,
        const QPropertyBindingSourceLocation& location  = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
        std::enable_if_t<std::is_invocable_v<Functor>>* = nullptr) {
        return setBinding(Qt::makePropertyBinding(std::forward<Functor>(f), location));
    }

    bool hasBinding() const {
        auto* bd = qGetBindingStorage(owner())->bindingData(this);
        return bd && bd->binding() != nullptr;
    }

    QPropertyBinding<T> binding() const {
        auto* bd = qGetBindingStorage(owner())->bindingData(this);
        return static_cast<QPropertyBinding<T>&&>(
            QUntypedPropertyBinding(bd ? bd->binding() : nullptr));
    }

    QPropertyBinding<T> takeBinding() { return setBinding(QPropertyBinding<T>()); }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f) {
        static_assert(std::is_invocable_v<Functor>,
                      "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, f);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f) {
        static_assert(std::is_invocable_v<Functor>,
                      "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(f);
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f) {
        static_assert(std::is_invocable_v<Functor>,
                      "Functor callback must be callable without any parameters");
        return QPropertyNotifier(*this, f);
    }

    const QtPrivate::QPropertyBindingData& bindingData() const {
        auto* storage = const_cast<QBindingStorage*>(qGetBindingStorage(owner()));
        return *storage->bindingData(const_cast<ThisType*>(this), true);
    }

private:
    void notify(const QtPrivate::QPropertyBindingData* binding) {
        if (binding) binding->notifyObservers(this, qGetBindingStorage(owner()));
        if constexpr (HasSignal) {
            if constexpr (SignalTakesValue::value)
                (owner()->*Signal)(this->valueBypassingBindings());
            else
                (owner()->*Signal)();
        }
    }
    Class* m_owner;
};

} // namespace qcm