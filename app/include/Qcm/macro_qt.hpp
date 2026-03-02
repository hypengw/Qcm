#pragma once

#include <QtCore/qtclasshelpermacros.h>

#ifndef Q_MOC_OUTPUT_REVISION
// This number should be in sync with moc's outputrevision.h
#define Q_MOC_OUTPUT_REVISION 69
#endif

// The following macros can be defined by tools that understand Qt
// to have the information from the macro.
#ifndef QT_ANNOTATE_CLASS
# define QT_ANNOTATE_CLASS(type, ...)
#endif
#ifndef QT_ANNOTATE_CLASS2
# define QT_ANNOTATE_CLASS2(type, a1, a2)
#endif
#ifndef QT_ANNOTATE_FUNCTION
# define QT_ANNOTATE_FUNCTION(x)
#endif
#ifndef QT_ANNOTATE_ACCESS_SPECIFIER
# define QT_ANNOTATE_ACCESS_SPECIFIER(x)
#endif

// The following macros are our "extensions" to C++
// They are used, strictly speaking, only by the moc.

#ifndef Q_MOC_RUN
#ifndef QT_NO_META_MACROS
# if defined(QT_NO_KEYWORDS)
#  define QT_NO_EMIT
# else
#   ifndef QT_NO_SIGNALS_SLOTS_KEYWORDS
#     define slots Q_SLOTS
#     define signals Q_SIGNALS
#   endif
# endif
# define Q_SLOTS QT_ANNOTATE_ACCESS_SPECIFIER(qt_slot)
# define Q_SIGNALS public QT_ANNOTATE_ACCESS_SPECIFIER(qt_signal)
# define Q_PRIVATE_SLOT(d, signature) QT_ANNOTATE_CLASS2(qt_private_slot, d, signature)
# define Q_EMIT
#ifndef QT_NO_EMIT
# define emit
#endif
#ifndef Q_CLASSINFO
# define Q_CLASSINFO(name, value)
#endif
#define Q_PLUGIN_METADATA(x) QT_ANNOTATE_CLASS(qt_plugin_metadata, x)
#define Q_INTERFACES(x) QT_ANNOTATE_CLASS(qt_interfaces, x)
#define Q_PROPERTY(...) QT_ANNOTATE_CLASS(qt_property, __VA_ARGS__)
#define Q_PRIVATE_PROPERTY(d, text) QT_ANNOTATE_CLASS2(qt_private_property, d, text)
#ifndef Q_REVISION
# define Q_REVISION(...)
#endif
#define Q_OVERRIDE(text) QT_ANNOTATE_CLASS(qt_override, text)
#define QDOC_PROPERTY(text) QT_ANNOTATE_CLASS(qt_qdoc_property, text)
#define Q_ENUMS(x) QT_ANNOTATE_CLASS(qt_enums, x)
#define Q_FLAGS(x) QT_ANNOTATE_CLASS(qt_enums, x)
#define Q_ENUM_IMPL(ENUM) \
    friend constexpr const QMetaObject *qt_getEnumMetaObject(ENUM) noexcept { return &staticMetaObject; } \
    friend constexpr const char *qt_getEnumName(ENUM) noexcept { return #ENUM; }
#define Q_ENUM(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_IMPL(x)
#define Q_FLAG(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_IMPL(x)
#define Q_ENUM_NS_IMPL(ENUM) \
    inline constexpr const QMetaObject *qt_getEnumMetaObject(ENUM) noexcept { return &staticMetaObject; } \
    inline constexpr const char *qt_getEnumName(ENUM) noexcept { return #ENUM; }
#define Q_ENUM_NS(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_NS_IMPL(x)
#define Q_FLAG_NS(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_NS_IMPL(x)
#define Q_SCRIPTABLE QT_ANNOTATE_FUNCTION(qt_scriptable)
#define Q_INVOKABLE  QT_ANNOTATE_FUNCTION(qt_invokable)
#define Q_SIGNAL QT_ANNOTATE_FUNCTION(qt_signal)
#define Q_SLOT QT_ANNOTATE_FUNCTION(qt_slot)
#define Q_MOC_INCLUDE(...) QT_ANNOTATE_CLASS(qt_moc_include, __VA_ARGS__)
#endif // QT_NO_META_MACROS

#ifndef QT_NO_TRANSLATION
// full set of tr functions
#  define QT_TR_FUNCTIONS \
    static inline QString tr(const char *s, const char *c = nullptr, int n = -1) \
        { return staticMetaObject.tr(s, c, n); }
#else
// inherit the ones from QObject
# define QT_TR_FUNCTIONS
#endif

#ifdef Q_QDOC
#define QT_TR_FUNCTIONS
#endif

#if defined(Q_CC_CLANG)
#  if Q_CC_CLANG >= 1100
#    define Q_OBJECT_NO_OVERRIDE_WARNING    QT_WARNING_DISABLE_CLANG("-Winconsistent-missing-override") QT_WARNING_DISABLE_CLANG("-Wsuggest-override")
#  elif Q_CC_CLANG >= 306
#    define Q_OBJECT_NO_OVERRIDE_WARNING    QT_WARNING_DISABLE_CLANG("-Winconsistent-missing-override")
#  endif
#elif defined(Q_CC_GNU) && Q_CC_GNU >= 501
#  define Q_OBJECT_NO_OVERRIDE_WARNING      QT_WARNING_DISABLE_GCC("-Wsuggest-override")
#elif defined(Q_CC_MSVC)
#  define Q_OBJECT_NO_OVERRIDE_WARNING      QT_WARNING_DISABLE_MSVC(26433)
#else
#  define Q_OBJECT_NO_OVERRIDE_WARNING
#endif

#if defined(Q_CC_GNU) && Q_CC_GNU >= 600
#  define Q_OBJECT_NO_ATTRIBUTES_WARNING    QT_WARNING_DISABLE_GCC("-Wattributes")
#else
#  define Q_OBJECT_NO_ATTRIBUTES_WARNING
#endif

#define QT_META_OBJECT_VARS \
    template <typename> static constexpr auto qt_create_metaobjectdata();       \
    template <typename MetaObjectTagType> static constexpr inline auto          \
    qt_staticMetaObjectContent = qt_create_metaobjectdata<MetaObjectTagType>(); \
    template <typename MetaObjectTagType> static constexpr inline auto          \
    qt_staticMetaObjectStaticContent = qt_staticMetaObjectContent<MetaObjectTagType>.staticData;\
    template <typename MetaObjectTagType> static constexpr inline auto          \
    qt_staticMetaObjectRelocatingContent = qt_staticMetaObjectContent<MetaObjectTagType>.relocatingData;

#define QT_OBJECT_GADGET_COMMON  \
    QT_META_OBJECT_VARS \
    Q_OBJECT_NO_ATTRIBUTES_WARNING \
    Q_DECL_HIDDEN static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **);

/* qmake ignore Q_OBJECT */
#define Q_OBJECT \
public: \
    QT_WARNING_PUSH \
    Q_OBJECT_NO_OVERRIDE_WARNING \
    static const QMetaObject staticMetaObject; \
    virtual const QMetaObject *metaObject() const; \
    virtual void *qt_metacast(const char *); \
    virtual int qt_metacall(QMetaObject::Call, int, void **); \
    QT_TR_FUNCTIONS \
private: \
    QT_OBJECT_GADGET_COMMON \
    QT_DEFINE_TAG_STRUCT(QPrivateSignal); \
    QT_WARNING_POP \
    QT_ANNOTATE_CLASS(qt_qobject, "")

/* qmake ignore Q_OBJECT */
#define Q_OBJECT_FAKE Q_OBJECT QT_ANNOTATE_CLASS(qt_fake, "")

#ifndef QT_NO_META_MACROS
/* qmake ignore Q_GADGET_EXPORT */
#define Q_GADGET_EXPORT(...) \
public: \
    static __VA_ARGS__ const QMetaObject staticMetaObject; \
    void qt_check_for_QGADGET_macro(); \
    typedef void QtGadgetHelper; \
private: \
    QT_WARNING_PUSH \
    QT_OBJECT_GADGET_COMMON \
    QT_WARNING_POP \
    QT_ANNOTATE_CLASS(qt_qgadget, "") \
    /*end*/

/* qmake ignore Q_GADGET */
#define Q_GADGET Q_GADGET_EXPORT()

    /* qmake ignore Q_NAMESPACE_EXPORT */
#define Q_NAMESPACE_EXPORT(...) \
    extern __VA_ARGS__ const QMetaObject staticMetaObject; \
    template <typename> static constexpr auto qt_create_metaobjectdata(); \
    QT_ANNOTATE_CLASS(qt_qnamespace, "") \
    /*end*/

/* qmake ignore Q_NAMESPACE */
#define Q_NAMESPACE Q_NAMESPACE_EXPORT() \
    /*end*/

#endif // QT_NO_META_MACROS

#else // Q_MOC_RUN
#define slots slots
#define signals signals
#define Q_SLOTS Q_SLOTS
#define Q_SIGNALS Q_SIGNALS
#define Q_CLASSINFO(name, value) Q_CLASSINFO(name, value)
#define Q_INTERFACES(x) Q_INTERFACES(x)
#define Q_PROPERTY(text) Q_PROPERTY(text)
#define Q_PRIVATE_PROPERTY(d, text) Q_PRIVATE_PROPERTY(d, text)
#define Q_REVISION(...) Q_REVISION(__VA_ARGS__)
#define Q_OVERRIDE(text) Q_OVERRIDE(text)
#define Q_ENUMS(x) Q_ENUMS(x)
#define Q_FLAGS(x) Q_FLAGS(x)
#define Q_ENUM(x) Q_ENUM(x)
#define Q_FLAG(x) Q_FLAG(x)
#define Q_ENUM_NS(x) Q_ENUM_NS(x)
#define Q_FLAG_NS(x) Q_FLAG_NS(x)
 /* qmake ignore Q_OBJECT */
#define Q_OBJECT Q_OBJECT
 /* qmake ignore Q_OBJECT */
#define Q_OBJECT_FAKE Q_OBJECT_FAKE
 /* qmake ignore Q_GADGET */
#define Q_GADGET Q_GADGET
#define Q_SCRIPTABLE Q_SCRIPTABLE
#define Q_INVOKABLE Q_INVOKABLE
#define Q_SIGNAL Q_SIGNAL
#define Q_SLOT Q_SLOT
#endif //Q_MOC_RUN


// forward declarations of structs and functions defined in QtQml
QT_BEGIN_NAMESPACE
namespace QQmlPrivate {
    template<typename, typename> struct QmlSingleton;
    template<class, class, bool> struct QmlAttached;
    template<class> struct QmlAttachedAccessor;
    template<class, class> struct QmlExtended;
    template<typename, typename> struct QmlInterface;
    template<class, class>
    struct QmlExtendedNamespace;
    template<class, class>
    struct QmlUncreatable;
    template<class, class>
    struct QmlAnonymous;
    template<class, class>
    struct QmlSequence;
    template<class, class>
    struct QmlResolved;
}

template <typename T> class QList;

template<typename... Args>
void qmlRegisterTypesAndRevisions(const char *uri, int versionMajor,
                                  QList<int> *qmlTypeIds = nullptr);

QT_END_NAMESPACE


#define QML_PRIVATE_NAMESPACE \
    QT_PREPEND_NAMESPACE(QQmlPrivate)

#define QML_REGISTER_TYPES_AND_REVISIONS \
    QT_PREPEND_NAMESPACE(qmlRegisterTypesAndRevisions)


#define QML_ELEMENT \
    Q_CLASSINFO("QML.Element", "auto")


#define QML_ANONYMOUS \
    Q_CLASSINFO("QML.Element", "anonymous") \
    enum class QmlIsAnonymous{yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlAnonymous; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_anonymous() {}

#define QML_NAMED_ELEMENT(NAME) \
    Q_CLASSINFO("QML.Element", #NAME)

#define QML_UNCREATABLE(REASON) \
    Q_CLASSINFO("QML.Creatable", "false") \
    Q_CLASSINFO("QML.UncreatableReason", REASON) \
    enum class QmlIsUncreatable {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlUncreatable; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_uncreatable() {}

#define QML_VALUE_TYPE(NAME) \
    Q_CLASSINFO("QML.Element", #NAME)

#define QML_CONSTRUCTIBLE_VALUE \
    Q_CLASSINFO("QML.Creatable", "true") \
    Q_CLASSINFO("QML.CreationMethod", "construct")

#define QML_STRUCTURED_VALUE \
    Q_CLASSINFO("QML.Creatable", "true") \
    Q_CLASSINFO("QML.CreationMethod", "structured")

#define QML_SINGLETON \
    Q_CLASSINFO("QML.Singleton", "true") \
    enum class QmlIsSingleton {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlSingleton; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_singleton() {}

#define QML_ADDED_IN_MINOR_VERSION(VERSION) \
    Q_CLASSINFO("QML.AddedInVersion", Q_REVISION(VERSION))

#define QML_ADDED_IN_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.AddedInVersion", Q_REVISION(MAJOR, MINOR))

#define QML_EXTRA_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.ExtraVersion", Q_REVISION(MAJOR, MINOR))

#define QML_REMOVED_IN_MINOR_VERSION(VERSION) \
    Q_CLASSINFO("QML.RemovedInVersion", Q_REVISION(VERSION))

#define QML_REMOVED_IN_VERSION(MAJOR, MINOR) \
    Q_CLASSINFO("QML.RemovedInVersion", Q_REVISION(MAJOR, MINOR))

#define QML_ATTACHED(ATTACHED_TYPE) \
    Q_CLASSINFO("QML.Attached", #ATTACHED_TYPE) \
    using QmlAttachedType = ATTACHED_TYPE; \
    template<class, class, bool> friend struct QML_PRIVATE_NAMESPACE::QmlAttached; \
    template<class> friend struct QML_PRIVATE_NAMESPACE::QmlAttachedAccessor; \
    inline constexpr void qt_qmlMarker_attached() {}

#define QML_EXTENDED(EXTENDED_TYPE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_TYPE) \
    using QmlExtendedType = EXTENDED_TYPE; \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlExtended; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_extended() {}

#define QML_EXTENDED_NAMESPACE(EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.ExtensionIsNamespace", "true") \
    static constexpr const QMetaObject *qmlExtendedNamespace() { return &EXTENDED_NAMESPACE::staticMetaObject; } \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlExtendedNamespace; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_extendedNamespace() {}

#define QML_NAMESPACE_EXTENDED(EXTENDED_NAMESPACE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_NAMESPACE)

#define QML_INTERFACE \
    Q_CLASSINFO("QML.Element", "anonymous") \
    enum class QmlIsInterface {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_interface() {}

#define QML_IMPLEMENTS_INTERFACES(INTERFACES) \
    Q_INTERFACES(INTERFACES) \
    enum class QmlIsInterface {yes = false}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlInterface;

#define QML_SEQUENTIAL_CONTAINER(VALUE_TYPE) \
    Q_CLASSINFO("QML.Sequence", #VALUE_TYPE) \
    using QmlSequenceValueType = VALUE_TYPE; \
    enum class QmlIsSequence {yes = true}; \
    template<typename, typename> friend struct QML_PRIVATE_NAMESPACE::QmlSequence; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_sequence() {}

#define QML_UNAVAILABLE \
    QML_FOREIGN(QQmlTypeNotAvailable)

#define QML_FOREIGN(FOREIGN_TYPE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_TYPE) \
    using QmlForeignType = FOREIGN_TYPE; \
    template<class, class> friend struct QML_PRIVATE_NAMESPACE::QmlResolved; \
    QT_WARNING_PUSH \
    QT_WARNING_DISABLE_GCC("-Wredundant-decls") \
    template<typename... Args> \
    friend void QML_REGISTER_TYPES_AND_REVISIONS(const char *uri, int versionMajor, QList<int> *); \
    QT_WARNING_POP \
    inline constexpr void qt_qmlMarker_foreign() {}

#define QML_FOREIGN_NAMESPACE(FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.Foreign", #FOREIGN_NAMESPACE) \
    Q_CLASSINFO("QML.ForeignIsNamespace", "true") \
    inline constexpr void qt_qmlMarker_foreign() {}

#define QML_CUSTOMPARSER \
    Q_CLASSINFO("QML.HasCustomParser", "true")

#define QML_USING(ORIGINAL) \
    using QmlUsing ## ORIGINAL = ORIGINAL; \
    Q_CLASSINFO("QML.Using", #ORIGINAL)

