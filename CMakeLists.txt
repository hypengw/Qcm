cmake_minimum_required(VERSION 3.28)

message(STATUS "PATH: $ENV{PATH}")

project(
  Qcm
  VERSION 1.3.0
  LANGUAGES CXX
  HOMEPAGE_URL "https://github.com/hypengw/Qcm"
  DESCRIPTION "Material You cloud music player")

set(CMAKE_CXX_COMPILER clang++ CACHE STRING "default clang++")

set(CMAKE_POLICY_DEFAULT_CMP0065 NEW) # remove unnecessary -rdynamic
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
set(WIN_DEFS WIN32_LEAN_AND_MEAN NOMINMAX NOGDI _WIN32_WINNT=0x0A00)
endif()

if(NOT ANDROID AND NOT WIN32)
  set(CMAKE_LINKER_TYPE "$<IF:$<CONFIG:Debug>,MOLD,DEFAULT>")
endif()
set(QT_QML_GENERATE_QMLLS_INI ON)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# options
option(QCM_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(QCM_USE_ASAN "use asan" OFF)
option(QCM_QML_DEBUG "qml debug" OFF)

# check lto
include(CheckIPOSupported)
check_ipo_supported(RESULT is_ipo_supported OUTPUT lto_error)
if(is_ipo_supported)
  message(STATUS "enable lto")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
endif()

if(UNIX AND NOT APPLE)
  set(LINUX CACHE INTERNAL TRUE "")
endif()

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Quick ShaderTools)

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

set(GC_SECTIONS_OPTS -fdata-sections -ffunction-sections)
set(GC_SECTIONS_LINK_OPTS "-Wl,--gc-sections")

add_compile_options(
  -fno-rtti
  -pthread
  ${WARN_OPTS}
  "$<$<NOT:$<CONFIG:Debug>>:${GC_SECTIONS_OPTS}>"
  "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
  # fix for clangd "$<$<CONFIG:Debug>:-fretain-comments-from-system-headers>"
  # asan
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=address>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=undefined>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize-recover=address>")

add_link_options(
  "$<$<NOT:$<CONFIG:Debug>>:${GC_SECTIONS_LINK_OPTS}>"
  # asan
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=address>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize=undefined>"
  "$<$<BOOL:${QCM_USE_ASAN}>:-fsanitize-recover=address>")

add_subdirectory(third_party)

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
add_subdirectory(crypto)

add_subdirectory(message)

add_subdirectory(mpris)
add_subdirectory(player)

set(QM_BUILD_STATIC ON)
if(ANDROID)
  set(QM_BUILD_EXAMPLE OFF)
endif()
add_subdirectory(app)

if(QCM_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()
