#pragma once

#define YCORE_EMPTY_OBJ
#define YCORE_EMPTY(...)
#define YCORE_PARENS          ()
#define YCORE_DEFER(m)        m YCORE_EMPTY()
#define YCORE_DEFER2(m)       m YCORE_EMPTY YCORE_EMPTY()()
#define YCORE_1(a, ...)       a
#define YCORE_2(a, b, ...)    b
#define YCORE_3(a, b, c, ...) c
#define YCORE_PASS(...)       __VA_ARGS__

#define _YCORE_VA_END        YCORE_EMPTY_OBJ, YCORE_EMPTY
#define _YCORE_VA_OPT(...)   YCORE_2(__VA_ARGS__)
#define YCORE_VA_OPT(a, ...) _YCORE_VA_OPT(_YCORE_VA_END##a, YCORE_PASS)

// expand 16 times
#define YCORE_EXPAND(arg)  YCORE_EXPAND1(YCORE_EXPAND1(YCORE_EXPAND1(YCORE_EXPAND1(arg))))
#define YCORE_EXPAND1(arg) YCORE_EXPAND2(YCORE_EXPAND2(YCORE_EXPAND2(YCORE_EXPAND2(arg))))
#define YCORE_EXPAND2(arg) arg

// clang-format off

#define YCORE_FOR_EACH(macro, ...) YCORE_FOR_EACH_EX(_YCORE_FOR_EACH_WRAPPER, macro, __VA_ARGS__)
#define _YCORE_FOR_EACH_WRAPPER(el, macro) macro(el)

//#define YCORE_FOR_EACH_EX(macro, extra, ...) __VA_OPT__(YCORE_EXPAND(_YCORE_FOR_EACH_HELPER(macro, extra, __VA_ARGS__)))
//#define _YCORE_FOR_EACH_HELPER(macro, extra, a1, ...)  macro(a1, extra) __VA_OPT__(_YCORE_FOR_EACH_AGAIN YCORE_PARENS (macro, extra, __VA_ARGS__))

#define YCORE_FOR_EACH_EX(macro, extra, ...) YCORE_VA_OPT(__VA_ARGS__)(YCORE_EXPAND(_YCORE_FOR_EACH_HELPER(macro, extra, __VA_ARGS__)))
#define _YCORE_FOR_EACH_HELPER(macro, extra, a1, ...) \
    macro(a1, extra) YCORE_VA_OPT(__VA_ARGS__)(YCORE_DEFER2(_YCORE_FOR_EACH_AGAIN)()(macro, extra, __VA_ARGS__))

#define _YCORE_FOR_EACH_AGAIN() _YCORE_FOR_EACH_HELPER
// clang-format on

#define C_DECLARE_PRIVATE(Class, DName)                                             \
    up<Private>            DName;                                                   \
    inline Class::Private* d_func() {                                               \
        return reinterpret_cast<Class::Private*>(ycore::GetPtrHelper(DName));       \
    }                                                                               \
    inline const Class::Private* d_func() const {                                   \
        return reinterpret_cast<const Class::Private*>(ycore::GetPtrHelper(DName)); \
    }

#define C_D(Class)       Class::Private* const d = d_func()
#define C_DP(Class, Ptr) Class::Private* const d = Ptr->d_func()

#define C_DECLARE_PUBLIC(Class, QName)                                              \
    inline Class*       q_func() { return static_cast<Class*>(QName); }             \
    inline const Class* q_func() const { return static_cast<const Class*>(QName); } \
    friend class Class;

#define C_Q(Class) Class* const q = q_func()

#if defined(_WIN32)
#    define C_DECL_EXPORT __declspec(dllexport)
#    define C_DECL_IMPORT __declspec(dllimport)
#else
#    define C_DECL_EXPORT __attribute__((visibility("default")))
#    define C_DECL_IMPORT __attribute__((visibility("default")))
#endif