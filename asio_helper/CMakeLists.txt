
add_library(asio_helper INTERFACE
    include/asio_helper/helper.h
    include/asio_helper/sync_file.h
    include/asio_helper/watch_dog.h
)
target_include_directories(asio_helper INTERFACE include)
target_link_libraries(asio_helper INTERFACE core asio)
