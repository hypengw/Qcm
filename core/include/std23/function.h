#ifndef INCLUDE_STD23_FUNCTION
#define INCLUDE_STD23_FUNCTION

#include "__functional_base.h"

#include <memory>
#include <new>

namespace std23
{

template<class Sig> struct _opt_fn_sig;

template<class R, class... Args> struct _opt_fn_sig<R(Args...)>
{
    using function_type = R(Args...);

    template<class... T>
    static constexpr bool is_invocable_using =
        std::is_invocable_r_v<R, T..., Args...>;
};

template<class R, class... Args> struct _copyable_function
{
    struct lvalue_callable
    {
        virtual R operator()(Args...) const = 0;
        virtual constexpr ~lvalue_callable() = default;

        void copy_into(std::byte *storage) const { copy_into_(storage); }
        void move_into(std::byte *storage) noexcept { move_into_(storage); }

      protected:
        virtual void copy_into_(void *) const = 0;
        virtual void move_into_(void *) noexcept = 0;
    };

    template<class Self> struct empty_object : lvalue_callable
    {
        void copy_into_(void *location) const override
        {
            ::new (location) Self;
        }

        void move_into_(void *location) noexcept override
        {
            ::new (location) Self;
        }
    };

    struct constructible_lvalue : lvalue_callable
    {
        [[noreturn]] R operator()(Args...) const override
        {
#if defined(_MSC_VER)
            __assume(0);
#else
            __builtin_unreachable();
#endif
        }
    };

    template<class T, class Self> class stored_object : constructible_lvalue
    {
        std::conditional_t<std::is_pointer_v<T>, T, std::unique_ptr<T>> p_;

      public:
        template<class F>
        explicit stored_object(F &&f)
            requires(_is_not_self<F, stored_object> and
                     not std::is_pointer_v<T>)
            : p_(std::make_unique<T>(std::forward<F>(f)))
        {}

        explicit stored_object(T p) noexcept requires std::is_pointer_v<T>
            : p_(p)
        {}

      protected:
        decltype(auto) get() const
        {
            if constexpr (std::is_pointer_v<T>)
                return p_;
            else
                return *p_;
        }

        void copy_into_(void *location) const override
        {
            ::new (location) Self(get());
        }

        void move_into_(void *location) noexcept override
        {
            ::new (location) Self(std::move(*this));
        }
    };

    template<class T, class Self>
    class stored_object<T &, Self> : constructible_lvalue
    {
        T &target_;

      public:
        explicit stored_object(T &target) noexcept : target_(target) {}

      protected:
        decltype(auto) get() const { return target_; }

        void copy_into_(void *location) const override
        {
            ::new (location) Self(*this);
        }

        void move_into_(void *location) noexcept override
        {
            ::new (location) Self(*this);
        }
    };

    struct empty_target_object final : empty_object<empty_target_object>
    {
        [[noreturn]] R operator()(Args...) const override
        {
            throw std::bad_function_call{};
        }
    };

    template<auto f>
    struct unbound_target_object final : empty_object<unbound_target_object<f>>
    {
        R operator()(Args... args) const override
        {
            return std23::invoke_r<R>(f, static_cast<Args>(args)...);
        }
    };

    template<class T>
    class target_object final : stored_object<T, target_object<T>>
    {
        using base = stored_object<T, target_object<T>>;

      public:
        template<class F>
        explicit target_object(F &&f) noexcept(
            std::is_nothrow_constructible_v<base, F>)
            requires _is_not_self<F, target_object>
            : base(std::forward<F>(f))
        {}

        R operator()(Args... args) const override
        {
            return std23::invoke_r<R>(this->get(), static_cast<Args>(args)...);
        }
    };

    template<auto f, class T>
    class bound_target_object final
        : stored_object<T, bound_target_object<f, T>>
    {
        using base = stored_object<T, bound_target_object<f, T>>;

      public:
        template<class U>
        explicit bound_target_object(U &&obj) noexcept(
            std::is_nothrow_constructible_v<base, U>)
            requires _is_not_self<U, bound_target_object>
            : base(std::forward<U>(obj))
        {}

        R operator()(Args... args) const override
        {
            return std23::invoke_r<R>(f, this->get(),
                                      static_cast<Args>(args)...);
        }
    };
};

template<class S, class = typename _opt_fn_sig<S>::function_type>
class function;

template<class S, class R, class... Args> class function<S, R(Args...)>
{
    using signature = _opt_fn_sig<S>;

    template<class... T>
    static constexpr bool is_invocable_using =
        signature::template is_invocable_using<T...>;

    template<class T> using lvalue = std::decay_t<T> &;

    using copyable_function = _copyable_function<R, _param_t<Args>...>;

    using lvalue_callable = copyable_function::lvalue_callable;
    using empty_target_object = copyable_function::empty_target_object;

    struct typical_target_object : lvalue_callable
    {
        union
        {
            void (*fp)() = nullptr;
            void *p;
        };
    };

    template<class F>
    using target_object_for =
        copyable_function::template target_object<std::unwrap_ref_decay_t<F>>;

    template<auto f>
    using unbound_target_object =
        copyable_function::template unbound_target_object<f>;

    template<auto f, class T>
    using bound_target_object_for =
        copyable_function::template bound_target_object<
            f, std::unwrap_ref_decay_t<T>>;

    template<class F, class FD = std::decay_t<F>>
    static bool constexpr is_viable_initializer =
        std::is_copy_constructible_v<FD> and std::is_constructible_v<FD, F>;

    alignas(typical_target_object)
        std::byte storage_[sizeof(typical_target_object)];

    auto storage_location() noexcept -> void * { return &storage_; }

    auto target() noexcept
    {
        return std::launder(reinterpret_cast<lvalue_callable *>(&storage_));
    }

    auto target() const noexcept
    {
        return std::launder(
            reinterpret_cast<lvalue_callable const *>(&storage_));
    }

  public:
    using result_type = R;

    function() noexcept { ::new (storage_location()) empty_target_object; }
    function(nullptr_t) noexcept : function() {}

    template<class F>
    function(F &&f) noexcept(
        std::is_nothrow_constructible_v<target_object_for<F>, F>)
        requires _is_not_self<F, function> and is_invocable_using<lvalue<F>> and
                 is_viable_initializer<F>
    {
        using T = target_object_for<F>;
        static_assert(sizeof(T) <= sizeof(storage_));

        if constexpr (_looks_nullable_to<F, function>)
        {
            if (f == nullptr)
            {
                std::construct_at(this);
                return;
            }
        }

        ::new (storage_location()) T(std::forward<F>(f));
    }

    template<auto f>
    function(nontype_t<f>) noexcept requires is_invocable_using<decltype(f)>
    {
        ::new (storage_location()) unbound_target_object<f>;
    }

    template<auto f, class U>
    function(nontype_t<f>, U &&obj) noexcept(
        std::is_nothrow_constructible_v<bound_target_object_for<f, U>, U>)
        requires is_invocable_using<decltype(f), lvalue<U>> and
                 is_viable_initializer<U>
    {
        using T = bound_target_object_for<f, U>;
        static_assert(sizeof(T) <= sizeof(storage_));

        ::new (storage_location()) T(std::forward<U>(obj));
    }

    function(function const &other) { other.target()->copy_into(storage_); }
    function(function &&other) noexcept { other.target()->move_into(storage_); }

    function &operator=(function const &other)
    {
        if (&other != this)
        {
            auto tmp = other;
            swap(tmp);
        }

        return *this;
    }

    function &operator=(function &&other) noexcept
    {
        if (&other != this)
        {
            std::destroy_at(this);
            return *std::construct_at(this, std::move(other));
        }
        else
            return *this;
    }

    void swap(function &other) noexcept { std::swap<function>(*this, other); }
    friend void swap(function &lhs, function &rhs) noexcept { lhs.swap(rhs); }

    ~function() { std::destroy_at(target()); }

    explicit operator bool() const noexcept
    {
        constexpr empty_target_object null;
        return __builtin_memcmp(storage_, (void *)&null, sizeof(void *)) != 0;
    }

    friend bool operator==(function const &f, nullptr_t) noexcept { return !f; }

    R operator()(Args... args) const
    {
        return (*target())(std::forward<Args>(args)...);
    }
};

template<class S> struct _strip_noexcept;

template<class R, class... Args> struct _strip_noexcept<R(Args...)>
{
    using type = R(Args...);
};

template<class R, class... Args> struct _strip_noexcept<R(Args...) noexcept>
{
    using type = R(Args...);
};

template<class S> using _strip_noexcept_t = _strip_noexcept<S>::type;

template<class F> requires std::is_function_v<F>
function(F *) -> function<_strip_noexcept_t<F>>;

template<class T>
function(T) -> function<_strip_noexcept_t<
                _drop_first_arg_to_invoke_t<decltype(&T::operator()), void>>>;

template<auto V>
function(nontype_t<V>)
    -> function<_strip_noexcept_t<_adapt_signature_t<decltype(V)>>>;

template<auto V, class T>
function(nontype_t<V>, T)
    -> function<_strip_noexcept_t<_drop_first_arg_to_invoke_t<decltype(V), T>>>;

} // namespace std23

#endif
