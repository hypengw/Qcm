# FetchDeps.cmake
#
# JSON-driven FetchContent driver. `include()` after project() and call
# fetchdeps() to declare/resolve dependencies from a deps.json manifest.
#
#     include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDeps.cmake)
#     fetchdeps(${CMAKE_CURRENT_SOURCE_DIR}/deps.json)
#
# The deps.json format is a valid flatpak-builder sources array. The
# `x-cmake` sidecar is an extension key used to carry CMake-specific
# metadata such as the dependency name or source_subdir override.
#
# Local source overrides
# ----------------------
# By default a dep `<name>` is taken from `<deps.json dir>/<name>` if
# that directory exists, otherwise it is fetched. To override per-dep,
# set the cache variable `FETCHDEPS_LOCAL_<name>=<path>` (typically
# from a CMakeUserPresets.json `cacheVariables` block, or `-D` on the
# command line). When set and the path exists it has the highest
# precedence; otherwise the lookup falls through to the deps.json dir
# / fetch.
#
# Selective fetch
# ---------------
#   NAMES <name> [<name> ...]
#     Whitelist. If non-empty, only entries whose `x-cmake.name` is in
#     the list are declared/fetched; others are skipped silently. Empty
#     (default) keeps the fetch-all behavior. A NAMES entry not present
#     in deps.json is reported as a warning.
#
# Skip-already-loaded
# -------------------
# A dep loaded by an earlier fetchdeps() call in the same configure is
# tracked as the directory property `_FETCHDEPS_LOADED` on
# CMAKE_SOURCE_DIR (one shared list for the whole tree). Subsequent
# calls hit a "<name> already loaded, skipping" log line instead of
# re-running add_subdirectory()/FetchContent_MakeAvailable().
#
# Example:
#
#     fetchdeps(${CMAKE_SOURCE_DIR}/deps.json)
#     fetchdeps(${CMAKE_SOURCE_DIR}/deps.json NAMES rstd)
#     # CMakeUserPresets.json cacheVariables:
#     #   "FETCHDEPS_LOCAL_qml_material": "${sourceDir}/../qml_material"

include_guard(GLOBAL)
include(FetchContent)

function(_fetchdeps_json_has_key out json key)
  string(JSON _t ERROR_VARIABLE _err TYPE "${json}" "${key}")
  if(_err)
    set(${out} FALSE PARENT_SCOPE)
  else()
    set(${out} TRUE PARENT_SCOPE)
  endif()
endfunction()

function(_fetchdeps_json_get_opt out json)
  string(JSON _v ERROR_VARIABLE _err GET "${json}" ${ARGN})
  if(_err)
    set(${out} "" PARENT_SCOPE)
  else()
    set(${out} "${_v}" PARENT_SCOPE)
  endif()
endfunction()

function(_fetchdeps_is_declared out name)
  get_property(_d GLOBAL PROPERTY _FETCHDEPS_DECLARED)
  if(name IN_LIST _d)
    set(${out} TRUE PARENT_SCOPE)
  else()
    set(${out} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(_fetchdeps_mark_declared name)
  set_property(GLOBAL APPEND PROPERTY _FETCHDEPS_DECLARED "${name}")
endfunction()

function(_fetchdeps_loaded_has out_var name)
  get_property(_v DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY _FETCHDEPS_LOADED)
  if(name IN_LIST _v)
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(_fetchdeps_loaded_add name)
  get_property(_v DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY _FETCHDEPS_LOADED)
  if(NOT name IN_LIST _v)
    list(APPEND _v "${name}")
    set_property(DIRECTORY "${CMAKE_SOURCE_DIR}"
                 PROPERTY _FETCHDEPS_LOADED "${_v}")
  endif()
endfunction()

macro(_fetchdeps_fetch_one _fd_entry _fd_source_root)
  string(JSON _fd_name  GET "${_fd_entry}" "x-cmake" name)
  string(JSON _fd_dtype GET "${_fd_entry}" type)

  _fetchdeps_mark_declared("${_fd_name}")

  set(_fd_did_load FALSE)
  _fetchdeps_loaded_has(_fd_loaded_hit "${_fd_name}")
  if(_fd_loaded_hit)
    message(STATUS "fetchdeps: ${_fd_name} already loaded, skipping")
  else()
    FetchContent_GetProperties(${_fd_name})
    if(${_fd_name}_POPULATED)
      message(STATUS "fetchdeps: ${_fd_name} already populated, skipping")
      set(_fd_did_load TRUE)
    elseif(DEFINED FETCHDEPS_LOCAL_${_fd_name}
           AND EXISTS "${FETCHDEPS_LOCAL_${_fd_name}}")
      message(STATUS "fetchdeps: ${_fd_name} <- local override ${FETCHDEPS_LOCAL_${_fd_name}}")
      add_subdirectory("${FETCHDEPS_LOCAL_${_fd_name}}" "${_fd_name}")
      set(_fd_did_load TRUE)
    elseif(EXISTS "${_fd_source_root}/${_fd_name}")
      message(STATUS "fetchdeps: ${_fd_name} <- local ${_fd_source_root}/${_fd_name}")
      add_subdirectory("${_fd_source_root}/${_fd_name}" "${_fd_name}")
      set(_fd_did_load TRUE)
    else()
    _fetchdeps_json_get_opt(_fd_dest "${_fd_entry}" dest)
    if(_fd_dest)
      set(_FETCHDEPS_DEST_${_fd_name} "${_fd_dest}" CACHE INTERNAL "" FORCE)
    endif()

    set(_fd_declare_args "")
    set(_fd_exclude_from_all FALSE)

    if(_fd_dtype STREQUAL "git")
      _fetchdeps_json_get_opt(_fd_url    "${_fd_entry}" url)
      _fetchdeps_json_get_opt(_fd_commit "${_fd_entry}" commit)
      _fetchdeps_json_get_opt(_fd_tag    "${_fd_entry}" tag)
      _fetchdeps_json_get_opt(_fd_branch "${_fd_entry}" branch)
      _fetchdeps_json_get_opt(_fd_dshallow "${_fd_entry}" "disable-shallow-clone")

      if(NOT _fd_url)
        message(FATAL_ERROR "fetchdeps: '${_fd_name}' type=git requires 'url'")
      endif()
      list(APPEND _fd_declare_args GIT_REPOSITORY "${_fd_url}")

      if(_fd_commit)
        list(APPEND _fd_declare_args GIT_TAG "${_fd_commit}")
      elseif(_fd_tag)
        list(APPEND _fd_declare_args GIT_TAG "${_fd_tag}")
      elseif(_fd_branch)
        list(APPEND _fd_declare_args GIT_TAG "${_fd_branch}")
      else()
        message(FATAL_ERROR
          "fetchdeps: '${_fd_name}' type=git requires one of commit/tag/branch")
      endif()

      if(_fd_dshallow STREQUAL "OFF")
        list(APPEND _fd_declare_args GIT_SHALLOW TRUE)
      else()
        list(APPEND _fd_declare_args GIT_SHALLOW FALSE)
      endif()

    elseif(_fd_dtype STREQUAL "archive" OR _fd_dtype STREQUAL "file")
      _fetchdeps_json_get_opt(_fd_url           "${_fd_entry}" url)
      _fetchdeps_json_get_opt(_fd_dest_filename "${_fd_entry}" "dest-filename")
      if(NOT _fd_url)
        message(FATAL_ERROR "fetchdeps: '${_fd_name}' type=${_fd_dtype} requires 'url'")
      endif()
      list(APPEND _fd_declare_args URL "${_fd_url}")
      if(_fd_dest_filename)
        list(APPEND _fd_declare_args DOWNLOAD_NAME "${_fd_dest_filename}")
      endif()
      if(_fd_dtype STREQUAL "file")
        list(APPEND _fd_declare_args DOWNLOAD_NO_EXTRACT TRUE)
      endif()

      set(_fd_hash "")
      foreach(_fd_algo sha512 sha256 sha1 md5)
        _fetchdeps_json_get_opt(_fd_v "${_fd_entry}" "${_fd_algo}")
        if(_fd_v)
          string(TOUPPER "${_fd_algo}" _fd_algo_upper)
          set(_fd_hash "${_fd_algo_upper}=${_fd_v}")
          break()
        endif()
      endforeach()
      if(NOT _fd_hash)
        message(FATAL_ERROR
          "fetchdeps: '${_fd_name}' type=${_fd_dtype} requires sha512/sha256/sha1/md5")
      endif()
      list(APPEND _fd_declare_args URL_HASH "${_fd_hash}")

    else()
      message(FATAL_ERROR "fetchdeps: '${_fd_name}' unsupported type '${_fd_dtype}'")
    endif()

    _fetchdeps_json_has_key(_fd_has_xc "${_fd_entry}" "x-cmake")
    if(_fd_has_xc)
      string(JSON _fd_xc GET "${_fd_entry}" "x-cmake")

      _fetchdeps_json_get_opt(_fd_v "${_fd_xc}" exclude_from_all)
      if(_fd_v)
        set(_fd_exclude_from_all TRUE)
        list(APPEND _fd_declare_args EXCLUDE_FROM_ALL)
      endif()

      _fetchdeps_json_get_opt(_fd_v "${_fd_xc}" find_package_args)
      if(_fd_v)
        separate_arguments(_fd_fpa UNIX_COMMAND "${_fd_v}")
        list(APPEND _fd_declare_args FIND_PACKAGE_ARGS ${_fd_fpa})
      endif()

      _fetchdeps_json_get_opt(_fd_v "${_fd_xc}" source_subdir)
      if(_fd_v)
        list(APPEND _fd_declare_args SOURCE_SUBDIR "${_fd_v}")
        set(_FETCHDEPS_SOURCE_SUBDIR_${_fd_name} "${_fd_v}" CACHE INTERNAL "" FORCE)
      endif()

      _fetchdeps_json_has_key(_fd_has_sub "${_fd_xc}" git_submodules)
      if(_fd_has_sub)
        string(JSON _fd_sub_len LENGTH "${_fd_xc}" git_submodules)
        if(_fd_sub_len GREATER 0)
          set(_fd_subs "")
          math(EXPR _fd_sub_last "${_fd_sub_len} - 1")
          foreach(_fd_i RANGE 0 ${_fd_sub_last})
            string(JSON _fd_s GET "${_fd_xc}" git_submodules ${_fd_i})
            list(APPEND _fd_subs "${_fd_s}")
          endforeach()
          list(APPEND _fd_declare_args GIT_SUBMODULES ${_fd_subs})
        endif()
      endif()
    endif()

    set(_FETCHDEPS_EXCLUDE_${_fd_name} "${_fd_exclude_from_all}" CACHE INTERNAL "" FORCE)

    message(STATUS "fetchdeps: ${_fd_name} <- fetch ${_fd_dtype}")
    FetchContent_Declare(${_fd_name} ${_fd_declare_args})
    FetchContent_MakeAvailable(${_fd_name})
    set(_fd_did_load TRUE)
    endif()
  endif()

  if(_fd_did_load)
    _fetchdeps_loaded_add("${_fd_name}")
  endif()
endmacro()

macro(fetchdeps _fd_deps_path)
  cmake_parse_arguments(_FD "" "" "NAMES" ${ARGN})
  if(_FD_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "fetchdeps: unexpected argument(s): ${_FD_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT EXISTS "${_fd_deps_path}")
    message(FATAL_ERROR "fetchdeps: ${_fd_deps_path} not found")
  endif()

  file(READ "${_fd_deps_path}" _fd_deps_json)
  string(JSON _fd_n ERROR_VARIABLE _fd_err LENGTH "${_fd_deps_json}")
  if(_fd_err)
    message(FATAL_ERROR "fetchdeps: ${_fd_deps_path} is not valid JSON: ${_fd_err}")
  endif()

  set_property(GLOBAL PROPERTY _FETCHDEPS_JSON_PATH "${_fd_deps_path}")

  get_filename_component(_fd_top_source_root "${_fd_deps_path}" DIRECTORY)

  if(_fd_n GREATER 0)
    math(EXPR _fd_top_last "${_fd_n} - 1")

    if(_FD_NAMES)
      set(_fd_known_names "")
      foreach(_fd_top_i RANGE 0 ${_fd_top_last})
        string(JSON _fd_top_entry GET "${_fd_deps_json}" ${_fd_top_i})
        string(JSON _fd_pre_name GET "${_fd_top_entry}" "x-cmake" name)
        list(APPEND _fd_known_names "${_fd_pre_name}")
      endforeach()
      foreach(_fd_want IN LISTS _FD_NAMES)
        if(NOT _fd_want IN_LIST _fd_known_names)
          message(WARNING
            "fetchdeps: NAMES entry '${_fd_want}' not in ${_fd_deps_path}")
        endif()
      endforeach()
    endif()

    foreach(_fd_top_i RANGE 0 ${_fd_top_last})
      string(JSON _fd_top_entry GET "${_fd_deps_json}" ${_fd_top_i})
      string(JSON _fd_pre_name GET "${_fd_top_entry}" "x-cmake" name)
      if(_FD_NAMES AND NOT _fd_pre_name IN_LIST _FD_NAMES)
        continue()
      endif()
      _fetchdeps_mark_declared("${_fd_pre_name}")
    endforeach()
    foreach(_fd_top_i RANGE 0 ${_fd_top_last})
      string(JSON _fd_top_entry GET "${_fd_deps_json}" ${_fd_top_i})
      string(JSON _fd_pre_name GET "${_fd_top_entry}" "x-cmake" name)
      if(_FD_NAMES AND NOT _fd_pre_name IN_LIST _FD_NAMES)
        continue()
      endif()
      _fetchdeps_fetch_one("${_fd_top_entry}" "${_fd_top_source_root}")
    endforeach()
  endif()
endmacro()
