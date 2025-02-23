#pragma once

#include <functional>

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QDateTime>

#include "core/core.h"
#include "core/qstr_helper.h"
#include "core/vec_helper.h"
#include "json_helper/helper.h"

#include "qcm_interface/type.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/export.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/model/mix.h"
#include "qcm_interface/model/radio.h"
#include "qcm_interface/model/program.h"
#include "qcm_interface/model/comment.h"

namespace qcm::model
{
using Extra = std::map<QString, QString, std::less<>>;

template<typename T>
using to_param =
    std::conditional_t<std::is_pointer_v<T>, T, std::add_lvalue_reference_t<std::add_const_t<T>>>;

} // namespace qcm::model

DECLARE_CONVERT(std::string, qcm::model::ItemId, QCM_INTERFACE_API);

DECLARE_JSON_SERIALIZER(QString, QCM_INTERFACE_API);
DECLARE_JSON_SERIALIZER(QUrl, QCM_INTERFACE_API);
DECLARE_JSON_SERIALIZER(QVariantMap, QCM_INTERFACE_API);
DECLARE_JSON_SERIALIZER(QDateTime, QCM_INTERFACE_API);
DECLARE_JSON_SERIALIZER(qcm::model::ItemId, QCM_INTERFACE_API);

JSON_SERIALIZER_NAMESPACE_BEGIN
template<typename T>
    requires requires(T t, qcm::json::njson j) {
        t.from_json(j);
        t.to_json(j);
    }
struct adl_serializer<T> {
    static void to_json(qcm::json::njson& j, const T& t) { t.to_json(j); }
    static void from_json(const qcm::json::njson& j, T& t) { t.from_json(j); }
};
JSON_SERIALIZER_NAMESPACE_END

namespace qcm::model
{

namespace details
{
struct ModelBase {};

template<typename T>
using model_base_t = std::conditional_t<std::is_void_v<T>, ModelBase, T>;
} // namespace details

constexpr auto MF_COPY { 1 };

template<typename T, typename TBase = void>
class Model : public details::model_base_t<TBase> {
    using Base             = details::model_base_t<TBase>;
    using is_base_copyable = std::integral_constant<bool, std::same_as<void, TBase> ||
                                                              std::is_copy_constructible_v<TBase>>;
    using is_base_moveable = std::integral_constant<bool, std::same_as<void, TBase> ||
                                                              std::is_move_constructible_v<TBase>>;

public:
    void from_json(const json::njson&);
    void to_json(json::njson&) const;

    bool operator==(const Model&) const;

protected:
    Model();
    ~Model();

    Model(const Model& o)
        requires std::is_copy_constructible_v<Base>
        : Model() {
        *this = o;
    }
    Model& operator=(const Model& o)
        requires std::is_copy_constructible_v<Base>
    {
        Base::operator=(o);
        assign(o);
        return *this;
    }

    Model(Model&& o) noexcept
        requires std::is_move_constructible_v<TBase>
        : Model() {
        *this = std::move(o);
    }
    Model& operator=(Model&& o) noexcept
        requires std::is_move_assignable_v<TBase>
    {
        Base::operator=(o);
        assign(std::move(o));
        return *this;
    }
    class Private;
    inline auto d_func() -> Private* { return m_ptr.get(); }
    inline auto d_func() const -> const Private* { return m_ptr.get(); };

private:
    struct Helper;
    void assign(const Model&);
    void assign(Model&&) noexcept;

    up<Private> m_ptr;
};

} // namespace qcm::model
