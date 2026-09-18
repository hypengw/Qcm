if(LITO_CMAKE_DEPENDENCY_MODE STREQUAL "find")
  find_package(effolkronium_random REQUIRED)
elseif(LITO_CMAKE_DEPENDENCY_MODE STREQUAL "source")
  add_subdirectory("${LITO_CMAKE_DEPENDENCY_SOURCE_DIR}"
                   "${CMAKE_CURRENT_BINARY_DIR}/random" EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "Unsupported lito cmake dependency mode: ${LITO_CMAKE_DEPENDENCY_MODE}")
endif()

if(NOT TARGET effolkronium_random)
  message(FATAL_ERROR "random did not provide effolkronium_random")
endif()

