#pragma once

#include <QObject>
#include "qcm_interface/model.h"

namespace qcm
{
namespace model
{

class QCM_INTERFACE_API Page : public Model<Page> {
    Q_GADGET
    DECLARE_MODEL(Page)
public:
    Page();
    ~Page();

    DECLARE_PROPERTY(QString, name, WRITE)
    DECLARE_PROPERTY(QString, icon, WRITE)
    DECLARE_PROPERTY(QString, source, WRITE)
    DECLARE_PROPERTY(bool, cache, WRITE)
    DECLARE_PROPERTY(bool, primary, WRITE)

    static constexpr std::tuple funcs {
        &Page::set_name, &Page::set_icon, &Page::set_source, &Page::set_cache, &Page::set_primary
    };

    template<std::size_t I, typename Arg>
    using helper = std::integral_constant<
        bool, std::invocable<typename std::tuple_element<I, decltype(funcs)>::type, Page, Arg>>;

    template<typename... TArgs>
        requires(ycore::make_index_sequence_helper<sizeof...(TArgs)>::template test<
                 helper, TArgs...>::value)
    Page(TArgs&&... args): Page() {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((this->*std::get<Is>(funcs))(args), ...);
        }(std::make_index_sequence<sizeof...(TArgs)> {});
    }
};

} // namespace model
} // namespace qcm
