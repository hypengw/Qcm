find_package(Qt6 REQUIRED COMPONENTS DBus)

add_library(
  mpris STATIC
  include/mpris/mpris.h include/mpris/mediaplayer2.h
  include/mpris/mediaplayer2_adaptor.h mpris.cpp mediaplayer2.cpp
  mediaplayer2_adaptor.cpp)

target_include_directories(mpris PUBLIC include)
target_link_libraries(mpris PUBLIC core Qt6::Core Qt6::DBus)
set_property(TARGET mpris PROPERTY POSITION_INDEPENDENT_CODE ON)
