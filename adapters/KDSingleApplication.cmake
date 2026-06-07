if(NOT LITO_CMAKE_DEPENDENCY_MODE STREQUAL "source")
  message(FATAL_ERROR "KDSingleApplication requires its source")
endif()

add_subdirectory("${LITO_CMAKE_DEPENDENCY_SOURCE_DIR}"
                 "${CMAKE_CURRENT_BINARY_DIR}/kd-single-application" EXCLUDE_FROM_ALL)

if(NOT TARGET kdsingleapplication)
  message(FATAL_ERROR "KDSingleApplication did not provide kdsingleapplication")
endif()

