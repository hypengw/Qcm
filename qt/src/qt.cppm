module;
#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QEvent>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QRunnable>
#include <QtCore/QObjectBindableProperty>
#include <QtCore/QDataStream>
#include <QtCore/QPluginLoader>
#include <QtCore/QPropertyData>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QLibrary>

#include <QtProtobuf/QtProtobuf>
#include <QtProtobuf/QProtobufSerializer>

#include <QtGui/QSurfaceFormat>

#include <QtQml/QJSValueIterator>
#include <QtQml/QQmlPropertyMap>
#include <QtQml/QQmlListProperty>
#include <QtQml/QQmlApplicationEngine>

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickImageProvider>
#include <QtQuick/QQuickAsyncImageProvider>
export module qt;

export using ::QString;
export using ::QAnyStringView;
export using ::QStringView;
export using ::QSize;
export using ::qint32;
export using ::qint64;
export using ::QDateTime;
export using ::qRgb;
export using ::QImage;
export using ::QUrl;
export using ::QUuid;
export using ::QUrlTwoFlags;
export using ::QDir;
export using ::QLibrary;

export using ::QTimer;
export using ::QEvent;
export using ::QThread;
export using ::QObject;
export using ::QMetaObject;
export using ::QObjectBindableProperty;
export using ::QPluginLoader;
export using ::QBindable;
export using ::QStandardPaths;
export using ::QProcess;

export using ::QGuiApplication;
export using ::QSurfaceFormat;
export using ::QWindowList;

export using ::QApplication;

export using ::QSettings;
export using ::QDataStream;
export using ::operator<<;
export using ::operator>>;

export using ::QJSValue;
export using ::QJSValueIterator;
export using ::QJsonValue;
export using ::QJsonObject;
export using ::QJsonDocument;
export using ::QQmlApplicationEngine;
export using ::QQmlPropertyMap;
export using ::QQmlEngine;
export using ::QJSEngine;
export using ::QQmlComponent;
export using ::QQmlListProperty;
export using ::qmlRegisterUncreatableType;

export using ::QQuickItem;
export using ::QQuickWindow;
#undef QT_PROPERTY_DEFAULT_BINDING_LOCATION
export constexpr auto QT_PROPERTY_DEFAULT_BINDING_LOCATION =
    QPropertyBindingSourceLocation(std::source_location::current());

export using ::QRunnable;
export using ::QPropertyData;
export using ::QPropertyBindingSourceLocation;
export using ::QUntypedPropertyData;
export using ::QPropertyNotifier;
export using ::QPropertyBinding;
export using ::QPropertyChangeHandler;
export using ::QUntypedPropertyBinding;
export using ::QBindingStorage;
export using ::qGetBindingStorage;

export using ::QVariantMap;
export using ::QAbstractItemModel;

export namespace Qt
{
using Qt::ConnectionType;
using Qt::makePropertyBinding;
} // namespace Qt

export namespace QtPrivate
{
using QtPrivate::QPropertyBindingData;
}

export namespace QTypeTraits
{
using QTypeTraits::is_dereferenceable;
using QTypeTraits::is_dereferenceable_v;
} // namespace QTypeTraits

export using ::QCoreApplication;

export using ::QQuickImageResponse;
export using ::QQuickTextureFactory;
export using ::QQuickImageProvider;
export using ::QQuickAsyncImageProvider;

namespace QtProtobuf
{
export using QtProtobuf::int64List;
export using QtProtobuf::int32;
export using QtProtobuf::int64;
} // namespace QtProtobuf
export using ::QProtobufSerializer;
