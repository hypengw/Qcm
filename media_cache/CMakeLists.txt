add_library(
  media_cache STATIC
  include/media_cache/media_cache.h
  include/media_cache/server.h
  include/media_cache/fragment.h
  get_request.h
  get_request.cpp
  media_cache.cpp
  server.cpp
  connection.h
  connection.cpp)

target_include_directories(media_cache PUBLIC include)

target_link_libraries(
  media_cache
  PUBLIC core asio_helper
  PRIVATE ctre request)
