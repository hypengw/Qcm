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
         src/helper/container.cppm)
target_sources(
  core
  PRIVATE include/core/core.h include/core/callable.h
          include/core/expected_helper.h include/core/log.h
          include/core/clangd.h src/log.cpp)

target_include_directories(core PUBLIC include)
target_link_libraries(
  core
  PUBLIC expected random fmt::fmt rstd::rstd
  PRIVATE platform)


# core.asio
add_library(core.asio SHARED src/asio/asio.cpp)
target_include_directories(core.asio PUBLIC include)
target_compile_definitions(
  core.asio
  PUBLIC ASIO_DYN_LINK ASIO_DISABLE_VISIBILITY
  PRIVATE QCM_ASIO_EXPORT)
target_link_libraries(core.asio PUBLIC core asio)
set_target_properties(core.asio PROPERTIES CXX_VISIBILITY_PRESET hidden
                                           OUTPUT_NAME qcm_asio)
install(TARGETS core.asio)
