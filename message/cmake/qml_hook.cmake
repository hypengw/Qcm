# Post-processes generated Qt protobuf headers in place:
#   - Replaces #include <QtQml/qqmlregistration.h> with Qcm/message/qml_hook.hpp
#   - Replaces QML_VALUE_TYPE(<name>) with QCM_MSG_MODEL_<name> for types that
#     need extra properties (see qml_hook.hpp).
#
# Idempotent: re-running is a no-op once the marker is present.

set(_marker "/* QCM_MSG_HOOKED */")

set(_files
  "${HEADERS_DIR}/model.qpb.h"
)

set(_inject_types album song artist mix providerStatus)

foreach(_f ${_files})
  if(NOT EXISTS "${_f}")
    message(WARNING "qml_hook: missing ${_f}")
    continue()
  endif()

  file(READ "${_f}" _content)
  if(_content MATCHES "QCM_MSG_HOOKED")
    continue()
  endif()

  string(REPLACE "#include <QtQml/qqmlregistration.h>"
    "#include \"Qcm/message/qml_hook.hpp\""
    _content "${_content}")

  foreach(_name ${_inject_types})
    string(REPLACE "QML_VALUE_TYPE(${_name})" "QCM_MSG_MODEL_${_name}"
      _content "${_content}")
  endforeach()

  set(_content "${_marker}\n${_content}")
  file(WRITE "${_f}" "${_content}")
  message(STATUS "qml_hook: patched ${_f}")
endforeach()
