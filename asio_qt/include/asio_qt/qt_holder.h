#pragma once
#include <QObject>
#include <QPointer>

namespace helper
{
template<typename... T>
    requires(std::is_base_of_v<QObject, T> && ...)
class QHolder : public QObject {
public:
    QHolder(bool override_parent, T*... args): QObject(nullptr), m_datas(args...) {
        (
            [override_parent](auto arg, auto self) {
                if (arg != nullptr) {
                    if (override_parent || arg->parent() == nullptr) {
                        arg->setParent(self);
                    }
                }
            }(args, this),
            ...);
    };
    QHolder(T*... args): QHolder(true, args...) {};
    ~QHolder() {};
    template<std::size_t I = 0>
    auto data() {
        return std::get<I>(m_datas);
    }

    template<std::size_t I = 0>
    auto pointer() {
        return QPointer { data<I>() };
    }

private:
    std::tuple<T*...> m_datas;
};
template<typename T>
auto create_qholder(T* data) -> std::shared_ptr<QHolder<T>> {
    return std::make_shared<QHolder<T>>(data);
}
template<typename T>
auto create_qholder(bool override_parent, T* data) -> std::shared_ptr<QHolder<T>> {
    return std::make_shared<QHolder<T>>(override_parent, data);
}
} // namespace helper