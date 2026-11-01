local qt = require("@lito.qt")

return function(target, qt6, proto, foreign_header)
  return qt.protobuf({
    target = target,
    qt = qt6,
    source = proto,
    proto_files = { "id.proto", "filter.proto", "message.proto", "model.proto", "entity.proto" },
    qml_proto_files = { "filter.proto", "message.proto", "model.proto", "entity.proto" },
    qml_moc_files = {{
      source = foreign_header,
      mode = "separate",
      output = "Qcm/message/moc_id_foreign.cpp",
    }},
    proto_includes = { "." },
    output = "Qcm/message",
    qml_uri = "Qcm.Msg",
    qml_version = "1.0",
  })
end
