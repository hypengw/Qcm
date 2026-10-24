local lito = require("@lito")
local qt = require("@lito.qt")
local target = lito.target({ kind = "test", name = "entity-store" })
qt.protobuf({
  target = target,
  qt = lito.external_dependency(target, "qt6"),
  source = lito.external_source(target, "qcm-proto"),
  proto_files = { "model.proto", "entity.proto" },
  proto_includes = { "." },
  output = "Qcm/message",
  qml_uri = "Qcm.StoreTest",
})

lito.target_add_generated_include(target, ".")
