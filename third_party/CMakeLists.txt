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
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(asio)
  target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
else()
  target_include_directories(asio INTERFACE ${ASIO_INCLUDE_DIRS})
endif()

# fmt
find_package(fmt 11.0.2 QUIET GLOBAL)
if(NOT fmt_FOUND)
  message(STATUS "fmt not found, fetching it...")
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.0.2
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(fmt)
endif()

# pegtl
find_package(pegtl 3.2.7 QUIET GLOBAL)
if(NOT pegtl_FOUND)
  message(STATUS "pegtl not found, fetching it...")
  FetchContent_Declare(
    pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG 3.2.8
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(pegtl)
endif()

# ctre
find_package(ctre 3.9.0 QUIET GLOBAL)
if(NOT ctre_FOUND)
  message(STATUS "ctre not found, fetching it...")
  FetchContent_Declare(
    ctre
    GIT_REPOSITORY
      https://github.com/hanickadot/compile-time-regular-expressions.git
    GIT_TAG v3.9.0
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(ctre)
endif()

# nlohmann json
set(JSON_DIAGNOSTICS ON)
find_package(nlohmann_json 3.11.0 QUIET GLOBAL)
if(NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann_json not found, fetching it...")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# cubeb
set(BUILD_TESTS OFF)
set(BUILD_TOOLS OFF)
set(BUNDLE_SPEEX ON)
set(USE_SANITIZERS OFF)
set(USE_WINMM OFF)
find_package(cubeb QUIET GLOBAL)
if(NOT cubeb_FOUND)
  message(STATUS "cubeb not found, fetching it...")
  FetchContent_Declare(
    cubeb
    GIT_REPOSITORY https://github.com/mozilla/cubeb.git
    GIT_COMMIT 78b2bce70e0d1c21d3c175b72f322c50801b2e94
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(cubeb)
endif()
if(NOT TARGET cubeb::cubeb)
  add_library(cubeb::cubeb ALIAS cubeb)
endif()


# ncrequest
find_package(ncrequest QUIET)
if(NOT ncrequest_FOUND)
  message(STATUS "ncrequest not found, fetching it...")
  FetchContent_Declare(
    ncrequest
    GIT_REPOSITORY https://github.com/hypengw/ncrequest.git
    GIT_TAG        748dd825b84f06d6398298029ea6828cbb44a107
    GIT_SHALLOW 1
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(ncrequest)
endif()