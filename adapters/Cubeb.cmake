if(NOT LITO_CMAKE_DEPENDENCY_MODE STREQUAL "source")
  message(FATAL_ERROR "cubeb requires its source")
endif()

add_subdirectory("${LITO_CMAKE_DEPENDENCY_SOURCE_DIR}"
                 "${CMAKE_CURRENT_BINARY_DIR}/cubeb" EXCLUDE_FROM_ALL)

if(NOT TARGET cubeb::cubeb)
  if(NOT TARGET cubeb)
    message(FATAL_ERROR "cubeb did not provide a library target")
  endif()
  add_library(cubeb::cubeb ALIAS cubeb)
endif()

