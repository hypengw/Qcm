#pragma once

#define C_DECLARE_PRIVATE_FUNC(Class, DName)                                        \
    inline Class::Private* d_func() {                                               \
        return reinterpret_cast<Class::Private*>(ycore::GetPtrHelper(DName));       \
    }                                                                               \
    inline const Class::Private* d_func() const {                                   \
        return reinterpret_cast<const Class::Private*>(ycore::GetPtrHelper(DName)); \
    }

#define C_DECLARE_PRIVATE(Class, DName) \
    up<Private> DName;                  \
    C_DECLARE_PRIVATE_FUNC(Class, DName)

#define C_D(Class) Class::Private* const d = d_func()
