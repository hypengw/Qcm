set(APP_ID io.github.hypengw.Qcm)
set(APP_NAME ${PROJECT_NAME})
set(APP_VERSION ${PROJECT_VERSION})
set(APP_SUMMARY ${PROJECT_DESCRIPTION})
set(APP_AUTHOR "hypeng")

find_package(Qt6 REQUIRED COMPONENTS Qml)

set(MODEL_SOURCES
    include/qcm_interface/model/user_account.h
    include/qcm_interface/model/app_info.h
    include/qcm_interface/model/busy_info.h
    include/qcm_interface/model/plugin_info.h
    include/qcm_interface/model/page.h
    include/qcm_interface/model/session.h
    include/qcm_interface/model/router_msg.h)

add_library(
  qcm_interface STATIC
  include/qcm_interface/async.h
  include/qcm_interface/action.h
  include/qcm_interface/notifier.h
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
  include/qcm_interface/duration.h
  include/qcm_interface/query.h
  include/qcm_interface/model/plugin_model.h
  include/qcm_interface/model/user_model.h
  include/qcm_interface/model/artist.h
  include/qcm_interface/model/song.h
  include/qcm_interface/model/album.h
  include/qcm_interface/model/mix.h
  include/qcm_interface/model/radio.h
  include/qcm_interface/model/program.h
  include/qcm_interface/model/comment.h
  include/qcm_interface/model/third_user.h
  include/qcm_interface/model/query_model.h
  include/qcm_interface/model/id_queue.h
  include/qcm_interface/model/empty_model.h
  include/qcm_interface/model/library.h
  include/qcm_interface/state/app_state.h
  action.cpp
  notifier.cpp
  api.cpp
  model.cpp
  item_id.cpp
  enum.cpp
  path.cpp
  router.cpp
  state.cpp
  global.cpp
  global_static.cpp
  plugin.cpp
  oper.cpp
  query.cpp
  sync_api.cpp
  meta_name.cpp
  duration.cpp
  model/plugin.cpp
  model/id_queue.cpp
  model/user_model.cpp
  ${MODEL_SOURCES})

target_sources(qcm_interface PUBLIC FILE_SET all TYPE CXX_MODULES FILES
                                    mod.cppm)

foreach(src ${MODEL_SOURCES})

  cmake_path(ABSOLUTE_PATH src BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  cmake_path(GET src STEM LAST_ONLY src_name)
  set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/gen")
  set(gen "${gen_dir}/${src_name}.cpp")

  set(model_generator "$<TARGET_FILE:qcm_model_generator>")
  set(model_generator_native "${model_generator}")
  if(ANDROID)
    set(model_generator "${QCM_MODEL_GENERATOR}")
  endif()
  add_custom_command(
    OUTPUT ${gen}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${gen_dir}"
    COMMAND "${model_generator}" "${src}" "${gen}"
    DEPENDS "${model_generator}" "${model_generator_native}" ${src})
  target_sources(qcm_interface PRIVATE ${gen})
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
             CXX_VISIBILITY_PRESET hidden
             CXX_SCAN_FOR_MODULES ON)

target_link_libraries(
  qcm_interface
  PUBLIC core
         error
         asio_helper
         asio_qt
         ncrequest::ncrequest
         json_helper
         meta_model
         Qt::Qml
  PRIVATE ctre::ctre)

install(TARGETS qcm_interface)
