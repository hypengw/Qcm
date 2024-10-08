set(APP_ID
    io.github.hypengw.Qcm)
set(APP_NAME
    ${PROJECT_NAME})
set(APP_VERSION
    ${PROJECT_VERSION})
set(APP_SUMMARY
    ${PROJECT_DESCRIPTION})
set(APP_AUTHOR
    "hypeng")

find_package(
  Qt6 REQUIRED
  COMPONENTS Qml)

set(MODEL_SOURCES
    include/qcm_interface/model/user_account.h
    include/qcm_interface/model/app_info.h
    include/qcm_interface/model/busy_info.h
    include/qcm_interface/model/plugin_info.h
    include/qcm_interface/model/page.h
    include/qcm_interface/model/session.h
    include/qcm_interface/model/router_msg.h
    )

add_library(
  qcm_interface SHARED
  include/qcm_interface/async.h
  include/qcm_interface/client.h
  include/qcm_interface/global.h
  include/qcm_interface/global_p.h
  include/qcm_interface/type.h
  include/qcm_interface/model.h
  include/qcm_interface/enum.h
  include/qcm_interface/api.h
  include/qcm_interface/item_id.h
  include/qcm_interface/export.h
  include/qcm_interface/path.h
  include/qcm_interface/router.h
  include/qcm_interface/plugin.h
  include/qcm_interface/model/plugin_model.h
  include/qcm_interface/model/user_model.h
  include/qcm_interface/state/app_state.h
  action.cpp
  api.cpp
  model.cpp
  item_id.cpp
  enum.cpp
  path.cpp
  router.cpp
  state.cpp
  global.cpp
  model/plugin.cpp
  model/user_model.cpp
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
    COMMAND "$<TARGET_FILE:qcm_model_generator>" "${src}" "${gen}"
    DEPENDS $<TARGET_FILE:qcm_model_generator> ${src})
  target_sources(
    qcm_interface
    PRIVATE ${gen})
endforeach()

target_include_directories(
  qcm_interface
  PUBLIC include
  PRIVATE include/qcm_interface .)

target_compile_definitions(
  qcm_interface
  PUBLIC APP_ID="${APP_ID}" APP_VERSION="${APP_VERSION}"
         APP_AUTHOR="${APP_AUTHOR}" APP_NAME="${APP_NAME}"
         APP_SUMMARY="${APP_SUMMARY}"
  PRIVATE QCM_INTERFACE_EXPORT QT_NO_SIGNALS_SLOTS_KEYWORDS)

set_target_properties(
  qcm_interface
  PROPERTIES AUTOMOC ON
             CXX_VISIBILITY_PRESET hidden)

target_link_libraries(
  qcm_interface
  PUBLIC core
         error
         asio_helper
         asio_qt
         request
         json_helper
         meta_model
         Qt::Qml
  PRIVATE
         ctre
        )

install(TARGETS qcm_interface)
