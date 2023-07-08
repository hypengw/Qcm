pkg_check_modules(
  LIBAV
  REQUIRED
  IMPORTED_TARGET
  libavformat
  libavcodec
  libavutil
  libswresample)

add_library(
  player STATIC
  include/player/player.h
  include/player/player_p.h
  player.cpp
  context.h
  audio_device.h
  audio_decoder.h
  stream_reader.h)

target_include_directories(
  player
  PUBLIC include
  PRIVATE .)

target_link_libraries(
  player
  PUBLIC core error asio_helper PkgConfig::LIBAV
  PRIVATE ctre cubeb::cubeb)
