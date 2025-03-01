add_library(core STATIC)

target_sources(
  core
  PUBLIC FILE_SET
         all
         TYPE
         CXX_MODULES
         BASE_DIRS
         src
         FILES
         src/log.cppm
         src/core.cppm
         src/helper.cppm
         src/container_helper.cppm
         src/str_helper.cppm)
target_sources(
  core
  PRIVATE include/core/core.h
          include/core/callable.h
          include/core/expected_helper.h
          include/core/variant_helper.h
          include/core/log.h
          src/log.cpp)

target_include_directories(core PUBLIC include)
target_link_libraries(
  core
  PUBLIC expected random fmt::fmt
  PRIVATE platform)
