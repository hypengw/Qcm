include(FetchContent)
find_package(PkgConfig REQUIRED)

# rstd
FetchContent_Declare(
  rstd
  GIT_REPOSITORY https://github.com/hypengw/rstd.git
  GIT_TAG a37763b19444cc4e6e26eddda2512754594613c3
  GIT_SHALLOW 0
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(rstd)

add_subdirectory(qr_code)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/meta_model")
  add_subdirectory(meta_model)
else()
  FetchContent_Declare(
    meta_model
    GIT_REPOSITORY https://github.com/hypengw/MetaModel.git
    GIT_TAG master
    GIT_SHALLOW 1
    GIT_PROGRESS 1)
  FetchContent_MakeAvailable(meta_model)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qml_material")
  add_subdirectory(qml_material)
else()
  FetchContent_Declare(
    qml_material
    GIT_REPOSITORY https://github.com/hypengw/QmlMaterial.git
    GIT_TAG main
    GIT_SHALLOW 1
    GIT_PROGRESS 1
    FIND_PACKAGE_ARGS GLOBAL)
  FetchContent_MakeAvailable(qml_material)
endif()

set(Random_BuildTests OFF)
FetchContent_Declare(
  random
  GIT_REPOSITORY https://github.com/ilqvya/random.git
  GIT_TAG 2b07d64ef1f8f00988ad813662ecb40766dba4cc
  GIT_SHALLOW 0
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(random)
add_library(random ALIAS effolkronium_random)

# KDSingleApplication
set(KDSingleApplication_QT6 ON)
set(KDSingleApplication_STATIC ON)
set(KDSingleApplication_TESTS OFF)
set(KDSingleApplication_EXAMPLES OFF)
set(KDSingleApplication_DOCS OFF)
FetchContent_Declare(
  KDSingleApplication
  GIT_REPOSITORY https://github.com/KDAB/KDSingleApplication.git
  GIT_TAG 631237acd4e20251c7f702db5e5434c83f0e336d
  GIT_SHALLOW 0
  GIT_PROGRESS 1
  SYSTEM EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS QUIET GLOBAL)
FetchContent_MakeAvailable(KDSingleApplication)

# asio
add_library(asio INTERFACE)
target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED ASIO_NO_TYPEID ASIO_HAS_THREADS ${WIN_DEFS})
pkg_check_modules(ASIO QUIET asio>=1.30.2)
if(NOT ASIO_FOUND)
  FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-34-0
    GIT_SHALLOW 1
    GIT_PROGRESS 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(asio)
  target_include_directories(asio SYSTEM
                             INTERFACE ${asio_SOURCE_DIR}/asio/include)
  message(STATUS "asio dir: ${asio_SOURCE_DIR}")
else()
  target_include_directories(asio SYSTEM INTERFACE ${ASIO_INCLUDE_DIRS})
endif()

# pegtl
FetchContent_Declare(
  pegtl
  GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
  GIT_TAG 3.2.8
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  SYSTEM EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 3.2.7 QUIET GLOBAL)
FetchContent_MakeAvailable(pegtl)

# ctre
FetchContent_Declare(
  ctre
  GIT_REPOSITORY
    https://github.com/hanickadot/compile-time-regular-expressions.git
  GIT_TAG e0f36500bc941fe447718ab625490728790c933f
  GIT_SHALLOW 0
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 3.9.0 GLOBAL)
FetchContent_MakeAvailable(ctre)

# cubeb
set(BUILD_TESTS OFF)
set(BUILD_TOOLS OFF)
set(BUNDLE_SPEEX ON)
set(USE_SANITIZERS OFF)
set(USE_WINMM OFF)
# find_package(cubeb QUIET GLOBAL)
FetchContent_Declare(
  cubeb
  GIT_REPOSITORY https://github.com/mozilla/cubeb.git
  GIT_TAG 78b2bce70e0d1c21d3c175b72f322c50801b2e94
  GIT_SHALLOW 0
  GIT_PROGRESS 1
  SYSTEM EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS GLOBAL)
FetchContent_MakeAvailable(cubeb)
if(NOT TARGET cubeb::cubeb)
  add_library(cubeb::cubeb ALIAS cubeb)
endif()

# ncrequest
FetchContent_Declare(
  ncrequest
  GIT_REPOSITORY https://github.com/hypengw/ncrequest.git
  GIT_TAG 56aa3326a6c7b25bb83ba537705d2a8f53552225
  GIT_SHALLOW 0
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(ncrequest)
