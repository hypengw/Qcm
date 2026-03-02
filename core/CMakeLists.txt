add_library(core STATIC)

target_sources(
  core
  PUBLIC FILE_SET
         all
         TYPE
         CXX_MODULES
         FILES
         src/mod.cppm
         src/random.cppm
         src/type_list.cppm
         src/log.cppm
         src/core.cppm
         src/mem.cppm
         src/lambda.cppm
         src/fmt.cppm
         src/helper/mod.cppm
         src/helper/str.cppm
         src/helper/container.cppm)
target_sources(core PRIVATE include/core/core.h include/core/callable.h
                            include/core/log.h src/log.cpp src/mem.cpp)

target_compile_definitions(core PUBLIC ${WIN_DEFS})
target_compile_features(core PUBLIC cxx_std_23)
target_include_directories(core PUBLIC include)
target_link_libraries(
  core
  PUBLIC random rstd::rstd
  PRIVATE platform)

# core.asio
add_library(core.asio STATIC src/asio/mod.cpp)
target_compile_definitions(
  core.asio
  PUBLIC ASIO_DISABLE_VISIBILITY
  PRIVATE QCM_ASIO_EXPORT)
target_link_libraries(core.asio PUBLIC core ncrequest::ncrequest asio)
set_target_properties(core.asio PROPERTIES CXX_VISIBILITY_PRESET hidden
                                           OUTPUT_NAME qcm_asio)
target_sources(
  core.asio
  PUBLIC FILE_SET
         all
         TYPE
         CXX_MODULES
         FILES
         src/asio/mod.cppm
         src/asio/task.cppm
         src/asio/sync_file.cppm
         src/asio/detached_log.cppm
         src/asio/watch_dog.cppm)
