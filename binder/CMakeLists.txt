add_library(binder STATIC include/binder/binder.h)

target_include_directories(
  binder
  PUBLIC include
  PRIVATE .)

target_link_libraries(binder PUBLIC core)
