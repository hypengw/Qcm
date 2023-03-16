add_library(error include/error/error.h)

target_include_directories(error PUBLIC include)
target_link_libraries(error PUBLIC core)
