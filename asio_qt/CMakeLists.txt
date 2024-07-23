add_library(asio_qt STATIC

    include/asio_qt/qt_execution_context.h
    include/asio_qt/qt_executor.h

    qt_execution_context.cpp
    qt_executor.cpp
)

target_include_directories(asio_qt PUBLIC include PRIVATE include/asio_qt)
target_link_libraries(asio_qt PUBLIC core asio_helper Qt6::Core PRIVATE request fmt::fmt)
