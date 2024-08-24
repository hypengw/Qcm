
add_library(
  platform STATIC
  include/platform/platform.h 
  platform.cpp)

target_include_directories(platform PUBLIC include)
target_link_libraries(platform PUBLIC core)
