include(FetchContent)
find_package(PkgConfig REQUIRED)
include(${CMAKE_SOURCE_DIR}/cmake/FetchDeps.cmake)

# random
set(Random_BuildTests OFF)

# KDSingleApplication
set(KDSingleApplication_QT6 ON)
set(KDSingleApplication_STATIC ON)
set(KDSingleApplication_TESTS OFF)
set(KDSingleApplication_EXAMPLES OFF)
set(KDSingleApplication_DOCS OFF)

# cubeb
set(BUILD_TESTS OFF)
set(BUILD_TOOLS OFF)
set(BUNDLE_SPEEX ON)
set(USE_SANITIZERS OFF)
set(USE_WINMM OFF)

# asio
add_library(asio INTERFACE)
target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED ASIO_NO_TYPEID
                                          ASIO_HAS_THREADS ${WIN_DEFS})

fetchdeps(${CMAKE_SOURCE_DIR}/deps.json)

pkg_check_modules(ASIO QUIET asio>=1.30.2)
if(NOT ASIO_FOUND)
  target_include_directories(asio SYSTEM
                             INTERFACE ${asio_SOURCE_DIR}/asio/include)
  message(STATUS "asio dir: ${asio_SOURCE_DIR}")
else()
  target_include_directories(asio SYSTEM INTERFACE ${ASIO_INCLUDE_DIRS})
endif()

add_library(random ALIAS effolkronium_random)
if(NOT TARGET cubeb::cubeb)
  add_library(cubeb::cubeb ALIAS cubeb)
endif()

add_subdirectory(qr_code)
