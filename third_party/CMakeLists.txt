include(FetchContent)

add_subdirectory(qr_code)

add_library(random INTERFACE)
target_include_directories(random INTERFACE random/include)

add_library(expected INTERFACE)
target_include_directories(expected INTERFACE expected/include)
add_library(tl::expected ALIAS expected)

add_library(asio INTERFACE)
# ASIO_ENABLE_HANDLER_TRACKING ASIO_NO_TYPEID
target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED)
target_include_directories(asio INTERFACE asio/asio/include)

set(KDSingleApplication_QT6 ON)
set(KDSingleApplication_STATIC ON)
set(KDSingleApplication_TESTS OFF)
set(KDSingleApplication_EXAMPLES OFF)
set(KDSingleApplication_DOCS OFF)
add_subdirectory(KDSingleApplication EXCLUDE_FROM_ALL)

# fmt
find_package(fmt 11.0.2 QUIET GLOBAL)
if (NOT fmt_FOUND)
    message(STATUS "fmt not found, fetching it...")
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.2
        GIT_SHALLOW 1
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(fmt)
endif()

# pegtl
find_package(pegtl 3.2.7 QUIET GLOBAL)
if (NOT pegtl_FOUND)
    message(STATUS "pegtl not found, fetching it...")
    FetchContent_Declare(
        pegtl
        GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
        GIT_TAG 3.2.8
        GIT_SHALLOW 1
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(pegtl)
endif()

# ctre
find_package(ctre 3.9.0 QUIET GLOBAL)
if (NOT ctre_FOUND)
    message(STATUS "ctre not found, fetching it...")
    FetchContent_Declare(
        ctre
        GIT_REPOSITORY https://github.com/hanickadot/compile-time-regular-expressions.git
        GIT_TAG v3.9.0
        GIT_SHALLOW 1
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(ctre)
endif()

# nlohmann json
set(JSON_DIAGNOSTICS ON)
find_package(nlohmann_json 3.11.0 QUIET GLOBAL)
if (NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching it...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW 1
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# cubeb
set(BUILD_TESTS OFF)
set(BUILD_TOOLS OFF)
set(BUNDLE_SPEEX ${ANDROID})
set(USE_SANITIZERS OFF)
set(USE_WINMM
    OFF
    CACHE BOOL "")
find_package(cubeb QUIET GLOBAL)
if (NOT cubeb_FOUND)
    message(STATUS "cubeb not found, fetching it...")
    FetchContent_Declare(
        cubeb
        GIT_REPOSITORY https://github.com/mozilla/cubeb.git
        GIT_COMMIT 78b2bce70e0d1c21d3c175b72f322c50801b2e94
        GIT_SHALLOW 1
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(cubeb)
endif()
add_library(cubeb::cubeb ALIAS cubeb)