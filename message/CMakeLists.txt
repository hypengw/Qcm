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
  QML_URI
  "Qcm.Msg")
target_include_directories(message PUBLIC "${output_inc_dir}")
target_link_libraries(message PUBLIC Qt6::ProtobufWellKnownTypes Qt6::ProtobufQuick)