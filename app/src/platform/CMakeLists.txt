add_library(platform STATIC platform.cpp win.cpp linux.cpp)

target_sources(
  platform
  PUBLIC FILE_SET
         all
         TYPE
         CXX_MODULES
         FILES
         platform.cppm)

target_link_libraries(platform PUBLIC rstd::rstd)
