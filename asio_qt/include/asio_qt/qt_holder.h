#pragma once
#include <QObject>

namespace helper
{
template<typename... T>
    requires(std::is_base_of_v<QObject, T> && ...)
class QHolder : public QObject {
public:
    QHolder(T*... args)
        : QObject(nullptr), m_datas(args...) {
              (
                  [](auto arg, auto self) {
                      if (arg != nullptr) arg->setParent(self);
                  }(args, this),
                  ...);
          };
    ~QHolder() {};
    template<std::size_t I = 0>
    auto data() {
        return std::get<I>(m_datas);
    }

private:
    std::tuple<T*...> m_datas;
};
template<typename T>
auto create_qholder(T* data) -> std::shared_ptr<QHolder<T>> {
    return std::make_shared<QHolder<T>>(data);
}
} // namespace helper