if(NOT LITO_CMAKE_DEPENDENCY_MODE STREQUAL "source")
  message(FATAL_ERROR "random requires its source")
endif()

add_subdirectory("${LITO_CMAKE_DEPENDENCY_SOURCE_DIR}"
                 "${CMAKE_CURRENT_BINARY_DIR}/random" EXCLUDE_FROM_ALL)

if(NOT TARGET effolkronium_random)
  message(FATAL_ERROR "random did not provide effolkronium_random")
endif()

