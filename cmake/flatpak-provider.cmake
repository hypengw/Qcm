include(FetchContent)

set(FLATPAK_SOURCE_OUTPUT "${CMAKE_BINARY_DIR}/flatpak_source.json" CACHE STRING "")

function(init_flatpak_source_file)
  set(flatpak_source
      "[{\"type\": \"inline\", \"dest-filename\": \"flatpak-provider.cmake\"}]")
  file(READ "${CMAKE_CURRENT_FUNCTION_LIST_FILE}" flatpak_provider_content)
  string(REPLACE "\\" "\\\\" flatpak_provider_content
                 "${flatpak_provider_content}")
  string(REPLACE "\"" "\\\"" flatpak_provider_content
                 "${flatpak_provider_content}")
  set(flatpak_source
      "[{
        \"type\": \"inline\",
        \"dest-filename\": \"flatpak-provider.cmake\",
        \"content\": \"${flatpak_provider_content}\"
    }]")
  file(WRITE "${FLATPAK_SOURCE_OUTPUT}" "${flatpak_source}")
endfunction()

init_flatpak_source_file()

function(to_flatpak_source dep_name)
  set(options)
  set(oneValueArgs EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR)
  set(multiValueArgs
      SOURCE_DIR
      BINARY_DIR
      # --- git ---
      GIT_REPOSITORY
      GIT_TAG
      GIT_SHALLOW
      GIT_REMOTE_NAME
      GIT_SUBMODULES
      GIT_SUBMODULES_RECURSE
      GIT_PROGRESS
      GIT_CONFIG
      GIT_REMOTE_UPDATE_STRATEGY
      # --- url ---
      URL
      URL_HASH
      URL_MD5
      DOWNLOAD_NAME
      DOWNLOAD_NO_EXTRACT
      DOWNLOAD_NO_PROGRESS
      TIMEOUT
      INACTIVITY_TIMEOUT
      HTTP_USERNAME
      HTTP_PASSWORD
      HTTP_HEADER
      TLS_VERSION
      TLS_VERIFY
      TLS_CAINFO
      NETRC
      NETRC_FILE
      # --- svn ---
      SVN_REPOSITORY
      SVN_REVISION
      SVN_USERNAME
      SVN_PASSWORD
      SVN_TRUST_CERT
      # ---- unsed ----
      HG_REPOSITORY # <url>
      HG_TAG # <tag>
      CVS_REPOSITORY # <cvsroot>
      CVS_MODULE # <mod>
      CVS_TAG # <tag>
      UPDATE_COMMAND # <cmd>...
      UPDATE_DISCONNECTED # <bool>
      PATCH_COMMAND # <cmd>...
      CONFIGURE_COMMAND # <cmd>...
      CMAKE_COMMAND # /.../cmake
      CMAKE_GENERATOR # <gen>
      CMAKE_GENERATOR_PLATFORM # <platform>
      CMAKE_GENERATOR_TOOLSET # <toolset>
      CMAKE_GENERATOR_INSTANCE # <instance>
      CMAKE_ARGS # <arg>...
      CMAKE_CACHE_ARGS # <arg>...
      CMAKE_CACHE_DEFAULT_ARGS # <arg>...
      SOURCE_SUBDIR # <dir>
      CONFIGURE_HANDLED_BY_BUILD # <bool>
      BUILD_COMMAND # <cmd>...
      BUILD_IN_SOURCE # <bool>
      BUILD_ALWAYS # <bool>
      BUILD_BYPRODUCTS # <file>...
      BUILD_JOB_SERVER_AWARE # <bool>
      INSTALL_COMMAND # <cmd>...
      INSTALL_BYPRODUCTS # <file>...
      INSTALL_JOB_SERVER_AWARE # <bool>
      TEST_COMMAND # <cmd>...
      TEST_BEFORE_INSTALL # <bool>
      TEST_AFTER_INSTALL # <bool>
      TEST_EXCLUDE_FROM_MAIN # <bool>
      LOG_DOWNLOAD # <bool>
      LOG_UPDATE # <bool>
      LOG_PATCH # <bool>
      LOG_CONFIGURE # <bool>
      LOG_BUILD # <bool>
      LOG_INSTALL # <bool>
      LOG_TEST # <bool>
      LOG_MERGED_STDOUTERR # <bool>
      LOG_OUTPUT_ON_FAILURE # <bool>
      USES_TERMINAL_DOWNLOAD # <bool>
      USES_TERMINAL_UPDATE # <bool>
      USES_TERMINAL_PATCH # <bool>
      USES_TERMINAL_CONFIGURE # <bool>
      USES_TERMINAL_BUILD # <bool>
      USES_TERMINAL_INSTALL # <bool>
      USES_TERMINAL_TEST # <bool>
      DEPENDS # <targets>...
      EXCLUDE_FROM_ALL # <bool>
      STEP_TARGETS # <step-target>...
      INDEPENDENT_STEP_TARGETS # <step-target>...
  )
  cmake_parse_arguments(EP "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  set(source_type "")
  set(url "")
  set(dest "build/_flatpak_deps/${dep_name}-src")

  set(tag "")
  set(branch "")
  set(commit "")

  set(md5 "")
  set(sha1 "")
  set(sha256 "")
  set(sha512 "")
  set(dest_filename "")
  set(disable_shallow_clone "false")
  set(disable_submodules "false")

  set(revision "")

  if(EP_GIT_REPOSITORY)
    set(url ${EP_GIT_REPOSITORY})
    set(source_type "git")
    if(EP_GIT_TAG)
      string(LENGTH "${EP_GIT_TAG}" len)
      if(${len} EQUAL 40)
        set(commit "${EP_GIT_TAG}")
      else()
        set(tag "${EP_GIT_TAG}")
      endif()
    endif()

    if(GIT_SHALLOW)
      if(GIT_SHALLOW EQUAL TRUE)
        set(disable_shallow_clone "false")
      else()
        set(disable_shallow_clone "true")
      endif()
    endif()

    cmake_policy(GET CMP0097 cmp0097)
    if("${cmp0097}" STREQUAL "NEW")
      set(disable_submodules "false")
      if(DEFINED EP_GIT_SUBMODULES)
        if(EP_GIT_SUBMODULES) # empty
          set(disable_submodules "true")
        endif()
      endif()
    endif()
  elseif(EP_URL)
    set(url ${EP_URL})
    if(EP_DOWNLOAD_NO_EXTRACT)
      set(source_type "file")
    else()
      set(source_type "archive")
    endif()

    if(EP_DOWNLOAD_NAME)
      set(dest_filename "${EP_DOWNLOAD_NAME}")
    endif()

    if(EP_URL_MD5)
      set(md5 "${EP_URL_MD5}")
    endif()
    if(EP_URL_HASH)
      set(hash_regex "^([a-zA-Z0-9]+)=(.+)$")
      string(REGEX REPLACE "${hash_regex}" "\\1" hash_type "${EP_URL_HASH}")
      string(REGEX REPLACE "${hash_regex}" "\\2" hash_val "${EP_URL_HASH}")
      if("${hash_type}" STREQUAL "sha1")
        set(sha1 ${hash_val})
      elseif("${hash_type}" STREQUAL "sha256")
        set(sha256 ${hash_val})
      elseif("${hash_type}" STREQUAL "sha512")
        set(sha512 ${hash_val})
      elseif("${hash_type}" STREQUAL "md5")
        set(md5 ${hash_val})
      else()
        message(WARNING "flatpak don't support source type: ${hash_type}")
      endif()
    endif()
  elseif(EP_SVN_REPOSITORY)
    set(url "${EP_SVN_REPOSITORY}")
    set(source_type "svn")
    if(EP_SVN_REVISION)
      set(revision "${EP_SVN_REVISION}")
    endif()
  endif()

  if(NOT source_type)
    message(FATAL_ERROR "flaptak only support FetchContent with git,url,svn")
  endif()

  set(source "{}")

  string(JSON source SET "${source}" url "\"${url}\"")
  string(JSON source SET "${source}" type \"${source_type}\")
  string(JSON source SET "${source}" dest \"${dest}\")

  if("${source_type}" STREQUAL "git")
    if(tag)
      string(JSON source SET "${source}" tag \"${tag}\")
    elseif(branch)
      string(JSON source SET "${source}" branch \"${branch}\")
    elseif(commit)
      string(JSON source SET "${source}" commit \"${commit}\")
    endif()
    string(JSON source SET "${source}" "disable-shallow-clone"
           "${disable_shallow_clone}")
  else()
    if(dest_filename)
      string(JSON source SET "${source}" "dest-filename" \"${dest_filename}\")
    endif()
    if(md5)
      string(JSON source SET "${source}" md5 \"${md5}\")
    elseif(sha512)
      string(JSON source SET "${source}" sha512 \"${sha512}\")
    elseif(sha256)
      string(JSON source SET "${source}" sha256 \"${sha256}\")
    elseif(sha1)
      string(JSON source SET "${source}" sha1 \"${sha1}\")
    endif()
  endif()

  file(READ "${FLATPAK_SOURCE_OUTPUT}" flatpak_source)
  string(JSON source_len LENGTH "${flatpak_source}")
  string(JSON flatpak_source SET "${flatpak_source}" ${source_len} "${source}")
  file(WRITE "${FLATPAK_SOURCE_OUTPUT}" "${flatpak_source}")
endfunction()

macro(flatpak_provider method dep_name)
  __fetchcontent_getsaveddetails("${dep_name}" fetch_details)
  message("flatpak_provider: ${dep_name}")
  to_flatpak_source("${dep_name}" ${fetch_details})

  if(DEFINED ENV{FLATPAK_ID})
    # flatpak-builder
    fetchcontent_setpopulated(
      ${dep_name}
      SOURCE_DIR
      "$ENV{FLATPAK_BUILDER_BUILDDIR}/build/_flatpak_deps/${dep_name}-src"
      BINARY_DIR
      "$ENV{FLATPAK_BUILDER_BUILDDIR}/build/_flatpak_deps/${dep_name}-build")
  else()
    # forward
    FetchContent_MakeAvailable(${dep_name})
  endif()

endmacro()

cmake_language(SET_DEPENDENCY_PROVIDER flatpak_provider SUPPORTED_METHODS
               FETCHCONTENT_MAKEAVAILABLE_SERIAL)
