if(LITO_CMAKE_DEPENDENCY_MODE STREQUAL "find")
  find_package(KDSingleApplication-qt6 REQUIRED)
  add_library(kdsingleapplication ALIAS KDAB::kdsingleapplication)
elseif(LITO_CMAKE_DEPENDENCY_MODE STREQUAL "source")
  add_subdirectory("${LITO_CMAKE_DEPENDENCY_SOURCE_DIR}"
                   "${CMAKE_CURRENT_BINARY_DIR}/kd-single-application" EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "Unsupported lito cmake dependency mode: ${LITO_CMAKE_DEPENDENCY_MODE}")
endif()

if(NOT TARGET kdsingleapplication)
  message(FATAL_ERROR "KDSingleApplication did not provide kdsingleapplication")
endif()

