if(NOT LITO_CMAKE_DEPENDENCY_MODE STREQUAL "source")
  message(FATAL_ERROR "qml-material requires its source")
endif()

if(NOT QML_MATERIAL_BUILD_TYPE STREQUAL "STATIC")
  message(FATAL_ERROR "qml-material must be built as a static QML module")
endif()

add_subdirectory("${LITO_CMAKE_DEPENDENCY_SOURCE_DIR}"
                 "${CMAKE_CURRENT_BINARY_DIR}/qml-material")

get_target_property(_qcm_qml_material_plugin_target
                    qml_material::qml_material QT_QML_MODULE_PLUGIN_TARGET)
if(NOT TARGET "${_qcm_qml_material_plugin_target}")
  message(FATAL_ERROR "qml-material static QML plugin target is unavailable")
endif()

add_library(qcm-qml-material INTERFACE)
target_link_libraries(qcm-qml-material INTERFACE
                      qml_material::qml_material
                      "${_qcm_qml_material_plugin_target}")
add_library(qml_material::qcm ALIAS qcm-qml-material)

unset(_qcm_qml_material_plugin_target)

