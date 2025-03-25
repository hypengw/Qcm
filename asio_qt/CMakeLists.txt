add_library(asio_qt STATIC

    include/asio_qt/qt_execution_context.h
    include/asio_qt/qt_executor.h
    include/asio_qt/qt_holder.h
    include/asio_qt/qt_sql.h
    include/asio_qt/qt_watcher.h

    qt_execution_context.cpp
    qt_executor.cpp
)

target_include_directories(asio_qt PUBLIC include PRIVATE include/asio_qt)
target_link_libraries(asio_qt PUBLIC core core.asio Qt6::Core PRIVATE ncrequest::ncrequest fmt::fmt)
