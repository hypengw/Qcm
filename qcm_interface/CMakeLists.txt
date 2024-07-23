find_package(
  Qt6 REQUIRED
  COMPONENTS Qml)

set(MODEL_SOURCES
    include/qcm_interface/model/user_account.h)

add_executable(
  qcm_interface_generator
  peg_cpp.hpp generator.cpp)
target_include_directories(
  qcm_interface_generator
  PRIVATE .)
target_link_libraries(
  qcm_interface_generator
  PRIVATE core pegtl Qt::Core)

add_library(
  qcm_interface SHARED
  include/qcm_interface/global.h
  include/qcm_interface/type.h
  include/qcm_interface/model.h
  include/qcm_interface/enum.h
  include/qcm_interface/api.h
  include/qcm_interface/item_id.h
  include/qcm_interface/export.h
  model.cpp
  item_id.cpp
  enum.cpp
  global.cpp
  ${MODEL_SOURCES})

foreach(
  src
  ${MODEL_SOURCES})

  cmake_path(ABSOLUTE_PATH src BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  cmake_path(GET src STEM LAST_ONLY src_name)
  set(gen_dir
      "${CMAKE_CURRENT_BINARY_DIR}/gen")
  set(gen
      "${gen_dir}/${src_name}.cpp")
  add_custom_command(
    OUTPUT ${gen}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${gen_dir}"
    COMMAND "$<TARGET_FILE:qcm_interface_generator>" "${src}" "${gen}"
    DEPENDS $<TARGET_FILE:qcm_interface_generator> ${src})
  target_sources(
    qcm_interface
    PRIVATE ${gen})
endforeach(src ${})

target_include_directories(
  qcm_interface
  PUBLIC include
  PRIVATE include/qcm_interface .)

target_compile_definitions(
  qcm_interface
  PRIVATE QCM_INTERFACE_EXPORT)

set_target_properties(
  qcm_interface
  PROPERTIES AUTOMOC ON
             CXX_VISIBILITY_PRESET hidden)
target_link_libraries(
  qcm_interface
  PUBLIC core asio_helper asio_qt request Qt::Qml)
