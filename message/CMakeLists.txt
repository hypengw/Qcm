find_package(Qt6 REQUIRED COMPONENTS Protobuf ProtobufWellKnownTypes)
set(CMAKE_AUTOMOC ON)

set(output_inc_dir "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/gen/Qcm/message")
qt_add_protobuf(message
    PROTO_FILES
        proto/message.proto
        proto/model.proto
    OUTPUT_DIRECTORY "${output_dir}"
)
target_include_directories(message PUBLIC "${output_inc_dir}")
target_link_libraries(message PUBLIC Qt6::ProtobufWellKnownTypes)
