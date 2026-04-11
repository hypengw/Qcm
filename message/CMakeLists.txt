find_package(Qt6 REQUIRED COMPONENTS Core Quick Protobuf ProtobufWellKnownTypes ProtobufQuick)
set(CMAKE_AUTOMOC ON)

set(output_inc_dir "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/gen/Qcm/message")

qt_add_protobuf(
  message
  PROTO_FILES
  proto/message.proto
  proto/model.proto
  proto/filter.proto
  OUTPUT_DIRECTORY
  "${output_dir}"
  OUTPUT_TARGETS
    message_targets
  QML
  #GENERATE_NON_FINAL_MESSAGES
  HEADER_GUARD
    pragma
  QML_URI
  "Qcm.Msg")
target_include_directories(message PUBLIC
  "${output_inc_dir}"
  "include"
  )
target_link_libraries(message PUBLIC Qt6::ProtobufWellKnownTypes Qt6::ProtobufQuick)
target_sources(message PRIVATE src/item_id.cpp)

# ----------------------------------------------------------------------------
# Post-process generated protobuf headers: inject custom Q_PROPERTY/methods by
# rewriting QML_VALUE_TYPE(<name>) to our per-type macro via qml_hook.cmake.
# The hook is idempotent (marker-guarded) and rewrites the file in place.
# ----------------------------------------------------------------------------
set(_hook_script "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qml_hook.cmake")
set(_hook_stamp  "${output_dir}/.qml_hook.stamp")
set(_hook_inputs "${output_dir}/model.qpb.h")

add_custom_command(
  OUTPUT "${_hook_stamp}"
  COMMAND ${CMAKE_COMMAND} -DHEADERS_DIR=${output_dir} -P "${_hook_script}"
  COMMAND ${CMAKE_COMMAND} -E touch "${_hook_stamp}"
  DEPENDS ${_hook_inputs} "${_hook_script}"
          "${CMAKE_CURRENT_SOURCE_DIR}/include/Qcm/message/qml_hook.hpp"
  VERBATIM
  COMMENT "Patching generated protobuf headers with QML hooks")

# Force AUTOMOC to wait for the hook so it re-scans the patched headers.
set_property(TARGET message APPEND PROPERTY
  AUTOGEN_TARGET_DEPENDS "${_hook_stamp}")

# Force recompilation of the generated .cpp files to follow the hook.
set_source_files_properties(
  "${output_dir}/model.qpb.cpp"
  "${output_dir}/model_qtprotoreg.cpp"
  "${output_dir}/message.qpb.cpp"
  "${output_dir}/message_qtprotoreg.cpp"
  "${output_dir}/filter.qpb.cpp"
  "${output_dir}/filter_qtprotoreg.cpp"
  TARGET_DIRECTORY message
  PROPERTIES OBJECT_DEPENDS "${_hook_stamp}")

target_sources(message PRIVATE "${_hook_stamp}")
set_source_files_properties("${_hook_stamp}"
  TARGET_DIRECTORY message
  PROPERTIES HEADER_FILE_ONLY TRUE GENERATED TRUE SKIP_AUTOMOC TRUE SKIP_AUTOGEN TRUE)