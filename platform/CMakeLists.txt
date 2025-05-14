add_library(platform STATIC src/platform.cpp src/win.cpp src/linux.cpp)

target_sources(
  platform
  PUBLIC FILE_SET
         all
         TYPE
         CXX_MODULES
         BASE_DIRS
         src
         FILES
         src/platform.cppm)

target_link_libraries(platform PUBLIC rstd::rstd)
