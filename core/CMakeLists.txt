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
         src/fmt.cppm
         src/helper/mod.cppm
         src/helper/str.cppm
         src/helper/container.cppm)
target_sources(
  core
  PRIVATE include/core/core.h include/core/callable.h
          include/core/expected_helper.h include/core/log.h
          include/core/clangd.h src/log.cpp)

target_compile_features(core PUBLIC cxx_std_23)
target_include_directories(core PUBLIC include)
target_link_libraries(
  core
  PUBLIC random rstd::rstd
  PRIVATE platform)

# core.asio
add_library(core.asio STATIC src/asio/asio.cpp)
target_compile_definitions(
  core.asio
  PUBLIC ASIO_DYN_LINK ASIO_DISABLE_VISIBILITY
  PRIVATE QCM_ASIO_EXPORT)
target_link_libraries(core.asio PUBLIC core asio)
set_target_properties(core.asio PROPERTIES CXX_VISIBILITY_PRESET hidden
                                           OUTPUT_NAME qcm_asio)
install(TARGETS core.asio)

# core.qasio
find_package(Qt6 REQUIRED COMPONENTS Core)
add_library(core.qasio STATIC src/qasio/qt_execution_context.cpp
                              src/qasio/qt_executor.cpp)

target_include_directories(core.qasio PRIVATE include/core/qasio)
target_link_libraries(
  core.qasio
  PUBLIC core.asio Qt6::Core
  PRIVATE ncrequest::ncrequest)
