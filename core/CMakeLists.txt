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
         src/mod.cppm
         src/random.cppm
         src/type_list.cppm
         src/log.cppm
         src/core.cppm
         src/lambda.cppm
         src/helper/mod.cppm
         src/helper/str.cppm
         src/helper/container.cppm
         )
target_sources(
  core
  PRIVATE include/core/core.h
          include/core/callable.h
          include/core/expected_helper.h
          include/core/log.h
          include/core/clangd.h
          src/log.cpp)

target_include_directories(core PUBLIC include)
target_link_libraries(
  core
  PUBLIC expected random fmt::fmt
  PRIVATE platform)
