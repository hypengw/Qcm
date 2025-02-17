
add_library(
  platform STATIC
  include/platform/platform.h 
  platform.cpp
  "$<$<PLATFORM_ID:Windows>:win.cpp>"
  "$<$<PLATFORM_ID:Linux>:linux.cpp>"
)

target_include_directories(platform PUBLIC include)
target_link_libraries(platform PUBLIC core)
