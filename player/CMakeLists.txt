set(FFMPEG_TARGETS)
if(ANDROID OR WIN32)
  find_package(ffmpeg REQUIRED)
  set(FFMPEG_TARGETS ffmpeg::avformat ffmpeg::avcodec ffmpeg::avutil
                     ffmpeg::swresample)
else()
  pkg_check_modules(
    LIBAV
    REQUIRED
    IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswresample)
  set(FFMPEG_TARGETS PkgConfig::LIBAV)
endif()

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
  PUBLIC core error core.asio ${FFMPEG_TARGETS} rstd::rstd
  PRIVATE ctre::ctre cubeb::cubeb 
    "$<$<PLATFORM_ID:Windows>:winmm>"
  )
