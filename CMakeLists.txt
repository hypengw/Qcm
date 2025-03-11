cmake_minimum_required(VERSION 3.28)

set(FLATPAK_SOURCE_OUTPUT "${CMAKE_SOURCE_DIR}/flaptak-source.json")
list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES cmake/flatpak-provider.cmake)
project(
  Qcm
  VERSION 1.2.0
  LANGUAGES CXX
  HOMEPAGE_URL "https://github.com/hypengw/Qcm"
  DESCRIPTION "Material You cloud music player")

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT ANDROID)
  set(CMAKE_LINKER_TYPE "$<IF:$<CONFIG:Debug>,MOLD,DEFAULT>")
endif()
set(QT_QML_GENERATE_QMLLS_INI ON)

# check lto
include(CheckIPOSupported)
check_ipo_supported(RESULT is_ipo_supported OUTPUT lto_error)
if(is_ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
endif()

if(UNIX AND NOT APPLE)
  set(LINUX CACHE INTERNAL TRUE "")
endif()

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Quick ShaderTools)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0002)
  qt_policy(SET QTP0002 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
  qt_policy(SET QTP0003 NEW)
  qt_policy(SET QTP0004 NEW)
endif()

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml_modules")

list(APPEND QML_DIRS "${QT_QML_OUTPUT_DIRECTORY}")
set(QML_IMPORT_PATH
    "${QML_DIRS}"
    CACHE STRING "Qt Creator extra qml import paths" FORCE)

option(QCM_USE_ASAN "use asan" OFF)
option(QCM_QML_DEBUG "qml debug" OFF)
if(ANDROID)
  set(QCM_MODEL_GENERATOR
      ""
      CACHE STRING "")
endif()

if("${QT_INSTALL_QML}" STREQUAL "")
  set(QT_INSTALL_QML "${CMAKE_INSTALL_LIBDIR}/qt6/qml")
endif()

set(QML_INSTALL_DIR
    "${QT_INSTALL_QML}"
    CACHE STRING "")
message("QML_INSTALL_DIR: ${QML_INSTALL_DIR}")

# -Wconversion -Wsign-conversion
set(WARN_OPTS
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-variable
    -Wno-unused-function
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-fconcepts-diagnostics-depth=2>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-dangling-reference>")

add_compile_options(
  "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
  # fix for clangd
  # "$<$<CONFIG:Debug>:-fretain-comments-from-system-headers>"
  # asan
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=address>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=undefined>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize-recover=address>")

add_link_options(
  # asan
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=address>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=undefined>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize-recover=address>")

add_subdirectory(third_party)

add_compile_options(${WARN_OPTS})

function(qt_install_qml_module target_)
  get_target_property(QML_TARGET_PATH ${target_} QT_QML_MODULE_TARGET_PATH)
  get_target_property(QML_TYEPINFO ${target_} QT_QML_MODULE_TYPEINFO)
  get_target_property(QML_PLUGIN_TARGET ${target_} QT_QML_MODULE_PLUGIN_TARGET)
  set(QML_TARGET_INSTALL_DIR "${QML_INSTALL_DIR}/${QML_TARGET_PATH}")

  install(TARGETS ${target_})

  install(TARGETS ${QML_PLUGIN_TARGET}
          LIBRARY DESTINATION ${QML_TARGET_INSTALL_DIR})

  install(FILES "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/qmldir"
                "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/${QML_TYEPINFO}"
          DESTINATION ${QML_TARGET_INSTALL_DIR})
endfunction()

function(register_qcm_service url_ target_)
  set_property(GLOBAL APPEND PROPERTY QCM_SERVICES_URL ${url_})
  set_property(GLOBAL APPEND PROPERTY QCM_SERVICES ${target_})
endfunction()

if(ANDROID)
  set(QCM_SERVICE_STATIC_BUILD ON)
endif()

add_subdirectory(platform)
add_subdirectory(core)
add_subdirectory(error)
add_subdirectory(json_helper)
add_subdirectory(asio_helper)
add_subdirectory(crypto)

add_subdirectory(message)
add_subdirectory(asio_qt)
add_subdirectory(meta_model)
add_subdirectory(generator/model)
add_subdirectory(qcm_interface)

add_subdirectory(mpris)
add_subdirectory(media_cache)
add_subdirectory(player)

set(QM_BUILD_STATIC ON)
if(ANDROID)
  set(QM_BUILD_EXAMPLE OFF)
endif()
add_subdirectory(qml_material)
add_subdirectory(app)