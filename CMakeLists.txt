cmake_minimum_required(VERSION 3.14)

project(
  Qcm
  VERSION 0.1
  LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(UNIX AND NOT APPLE)
  set(LINUX CACHE INTERNAL TRUE "")
endif()

find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Quick)
qt_standard_project_setup()

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml_modules")

list(APPEND QML_DIRS "${QT_QML_OUTPUT_DIRECTORY}")
set(QML_IMPORT_PATH
    "${QML_DIRS}"
    CACHE STRING "Qt Creator extra qml import paths" FORCE)

# -Wconversion -Wsign-conversion
add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Wno-unused-variable
  -Wno-unused-function
  -Wno-dangling-reference
  -fconcepts-diagnostics-depth=2)

add_compile_options(
  $<$<CONFIG:Debug>:-ggdb> $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
  # $<$<CONFIG:Debug>:-fsanitize=address>
)

# add_link_options( $<$<CONFIG:Debug>:-fsanitize=address> )

add_subdirectory(third_party)
add_subdirectory(core)
add_subdirectory(error)
add_subdirectory(material_helper)
add_subdirectory(json_helper)
add_subdirectory(asio_helper)
add_subdirectory(asio_qt)
add_subdirectory(request)
add_subdirectory(crypto)
add_subdirectory(service/ncm)
add_subdirectory(mpris)
add_subdirectory(meta_model)
add_subdirectory(media_cache)
add_subdirectory(player)
add_subdirectory(qml_material)
add_subdirectory(app)
