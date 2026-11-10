module;
#include <rstd/macro.hpp>
export module qcm.backend:db.sync_batch.schema;
export import rstd;
export import rstd.serde;
export import :db.error.write;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::sync_schema
{
template<class T>
using Definition = decltype(qcm_sync_definition(static_cast<T*>(nullptr)));
}

export namespace qcm::backend
{
template<class T>
class Patch {
    Option<T> value_;
    explicit Patch(Option<T> value): value_(rstd::move(value)) {}

public:
    Patch() = default;
    static auto Missing() -> Patch { return Patch {}; }
    static auto Set(T value) -> Patch { return Patch(Some(rstd::move(value))); }
    auto        is_missing() const -> bool { return value_.is_none(); }
    auto        is_set() const -> bool { return value_.is_some(); }
    auto        value() const& -> const T& { return *value_; }
    auto        value() & -> T& { return *value_; }
    auto        value() const&& -> const T& = delete;
    auto        value() && -> T&& { return rstd::move(*value_); }
};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]] Mask {
    [[pmacro::helper("serde::derive", default)]]
    Option<Vec<String>> include;
    [[pmacro::helper("serde::derive", default)]]
    Option<Vec<String>> exclude;
    auto                allows(ref<str> name) const -> bool {
        auto contains = [name](const Vec<String>& fields) {
            for (const auto& field : fields.as_slice())
                if (field.as_str() == name) return true;
            return false;
        };
        return (include.is_none() || contains(*include)) &&
               (exclude.is_none() || ! contains(*exclude));
    }
    template<class Input>
    auto validate() const -> DbWriteResult<empty> {
        if (include.is_some() && exclude.is_some())
            return Err(DbWriteError::Invalid(
                String::make("include and exclude are mutually exclusive"_str)));
        auto validate_names = [](const Option<Vec<String>>& fields) -> DbWriteResult<empty> {
            if (fields.is_none()) return Ok(empty {});
            for (const auto& field : fields->as_slice()) {
                if (! sync_schema::Definition<Input>::contains(field.as_str()))
                    return Err(
                        DbWriteError::Invalid(rstd::fmt::format("unknown mask field: {}", field)));
            }
            return Ok(empty {});
        };
        rstd_try(validate_names(include));
        return validate_names(exclude);
    }
};
} // namespace qcm::backend

export namespace rstd
{
template<class T>
struct Impl<serde::Deserialize, qcm::backend::Patch<T>> {
    template<class D>
    static auto deserialize(D& source) -> Result<qcm::backend::Patch<T>, typename D::error_type> {
        return Ok(qcm::backend::Patch<T>::Set(rstd_try(serde::deserialize<T>(source))));
    }
};
} // namespace rstd

export namespace qcm::backend::sync_schema
{
template<class T>
auto finite_patch(const T&) -> bool {
    return true;
}
template<class T>
auto finite_patch(const Patch<T>& value) -> bool {
    if constexpr (rstd::mtp::same_as<T, f64>)
        return value.is_missing() || value.value().is_finite();
    else
        return true;
}
template<class... Values>
auto validate_patches(const Values&... values) -> Result<empty, ref<str>> {
    if (! (finite_patch(values) && ...)) return Err("number must be finite"_str);
    return Ok(empty {});
}

struct CloneValue {
    static auto convert(const auto& value) { return rstd::as<rstd::clone::Clone>(value).clone(); }
};

template<auto Pointer, auto ModelPointer, rstd::serde::Text Name, class Conversion = CloneValue>
struct Field {
    static constexpr auto name() { return Name.view(); }
    template<class Changes, class Model>
    static auto apply_changes(Changes& changes, const Model& model, const Mask& mask) -> bool {
        if (! mask.allows(Name.view())) return false;
        changes.template set<ModelPointer>(
            rstd::as<rstd::clone::Clone>(model.*ModelPointer).clone());
        return true;
    }
    template<class Model, class Input>
    static auto apply(Model& model, const Input& input, const Mask& mask, bool inserted) -> void {
        const auto& patch = input.*Pointer;
        if (patch.is_set() && (inserted || mask.allows(Name.view())))
            model.*ModelPointer = Conversion::convert(patch.value());
    }
};

template<auto Pointer, rstd::serde::Text Name>
struct Extra {
    static constexpr auto name() { return Name.view(); }
};

template<class T, class... Members>
struct Fields {
    template<rstd::size_t I>
    using member_at = rstd::mtp::tuple_element<I, rstd::tuple<Members...>>;
    static auto contains(ref<str> name) -> bool { return ((name == Members::name()) || ...); }
    template<class Changes, class Model>
    static auto apply_changes(Changes& changes, const Model& model, const Mask& mask) -> bool {
        bool changed = false;
        ((changed = Members::apply_changes(changes, model, mask) || changed), ...);
        return changed;
    }
};

template<class T, class InsertFields, class MappedFields, class... Extras>
struct Entity;

template<class T, class InsertFields, class... Members, class... Extras>
struct Entity<T, InsertFields, Fields<T, Members...>, Extras...> {
    static auto contains(ref<str> name) -> bool {
        return ((name == Members::name()) || ...) || ((name == Extras::name()) || ...);
    }
    template<class Model, rstd::size_t... I>
    static auto apply_fields(Model& model, const T& input, const Mask& mask, bool inserted,
                             rstd::mtp::index_sequence<I...>) -> void {
        auto apply = [&]<rstd::size_t Index>() {
            using Field       = rstd::mtp::tuple_element<Index, rstd::tuple<Members...>>;
            using InsertField = typename Definition<InsertFields>::template member_at<Index>;
            if (inserted && input.on_insert.is_some())
                InsertField::apply(model, *input.on_insert, mask, true);
            Field::apply(model, input, mask, inserted);
        };
        (apply.template operator()<I>(), ...);
    }
    template<class Model>
    static auto apply(Model& model, const T& input, const Mask& mask, bool inserted)
        -> DbWriteResult<empty> {
        rstd_try(mask.template validate<T>());
        apply_fields(
            model, input, mask, inserted, rstd::mtp::make_index_sequence<sizeof...(Members)> {});
        return Ok(empty {});
    }
};
} // namespace qcm::backend::sync_schema
