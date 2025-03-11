include(FetchContent)
find_package(PkgConfig REQUIRED)

add_subdirectory(qr_code)

FetchContent_Declare(
  qml_material
  GIT_REPOSITORY https://github.com/hypengw/QmlMaterial.git
  GIT_TAG b800e6893363c290882c82b49551e2634126ff61
  GIT_SHALLOW 1
  GIT_PROGRESS 1)
FetchContent_MakeAvailable(qml_material)

set(Random_BuildTests OFF)
FetchContent_Declare(
  random
  GIT_REPOSITORY https://github.com/ilqvya/random.git
  GIT_TAG 041302cdb6668c1f6b09e803b6c9bda8e3820fdc
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(random)
add_library(random ALIAS effolkronium_random)

set(EXPECTED_BUILD_PACKAGE OFF)
set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE_DEB OFF)
FetchContent_Declare(
  expected
  GIT_REPOSITORY https://github.com/TartanLlama/expected.git
  GIT_TAG 41d3e1f48d682992a2230b2a715bca38b848b269
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(expected)

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
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  SYSTEM EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS QUIET GLOBAL)
FetchContent_MakeAvailable(KDSingleApplication)

# asio
add_library(asio INTERFACE)
target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED)
pkg_check_modules(ASIO QUIET asio>=1.30.2)
if(NOT ASIO_FOUND)
  message(STATUS "asio not found, fetching it...")
  FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
    GIT_SHALLOW 1
    GIT_PROGRESS 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(asio)
  target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
else()
  target_include_directories(asio INTERFACE ${ASIO_INCLUDE_DIRS})
endif()

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.0.2
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 11.0.2 QUIET GLOBAL)
FetchContent_MakeAvailable(fmt)

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
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 3.9.0 GLOBAL)
FetchContent_MakeAvailable(ctre)

# nlohmann json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  SYSTEM EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 3.11.0 GLOBAL)
FetchContent_MakeAvailable(nlohmann_json)

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
  GIT_COMMIT 78b2bce70e0d1c21d3c175b72f322c50801b2e94
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  SYSTEM EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS GLOBAL)
FetchContent_MakeAvailable(cubeb)
if(NOT TARGET cubeb::cubeb)
  add_library(cubeb::cubeb ALIAS cubeb)
endif()

# trait
FetchContent_Declare(
  trait
  GIT_REPOSITORY https://github.com/hypengw/trait.git
  GIT_TAG 86d8384ceffe61f6167be87c5aeb16257d818fd3
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(trait)

# ncrequest
FetchContent_Declare(
  ncrequest
  GIT_REPOSITORY https://github.com/hypengw/ncrequest.git
  GIT_TAG 7824860fb0054e9d26f1214b7cb93d8dffd2420a
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(ncrequest)
