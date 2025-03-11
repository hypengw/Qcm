include(FetchContent)
find_package(PkgConfig REQUIRED)

add_subdirectory(qr_code)

add_library(random INTERFACE)
target_include_directories(random INTERFACE random/include)

add_library(expected INTERFACE)
target_include_directories(expected INTERFACE expected/include)
add_library(tl::expected ALIAS expected)

set(KDSingleApplication_QT6 ON)
set(KDSingleApplication_STATIC ON)
set(KDSingleApplication_TESTS OFF)
set(KDSingleApplication_EXAMPLES OFF)
set(KDSingleApplication_DOCS OFF)
add_subdirectory(KDSingleApplication EXCLUDE_FROM_ALL)

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
  EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 3.2.7 QUIET GLOBAL)
FetchContent_MakeAvailable(pegtl)

# ctre find_package(ctre 3.9.0 QUIET GLOBAL)
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
  EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 3.11.0 GLOBAL)
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
  EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS GLOBAL)
FetchContent_MakeAvailable(cubeb)

if(NOT TARGET cubeb::cubeb)
  add_library(cubeb::cubeb ALIAS cubeb)
endif()

# ncrequest
FetchContent_Declare(
  ncrequest
  GIT_REPOSITORY https://github.com/hypengw/ncrequest.git
  GIT_TAG 7824860fb0054e9d26f1214b7cb93d8dffd2420a
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS GLOBAL)
FetchContent_MakeAvailable(ncrequest)
