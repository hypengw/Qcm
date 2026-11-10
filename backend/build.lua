local target = lito.target({ kind = "lib", name = "qcm-backend-cpp" })
local protoc = lito.tool("protoc")
local generator = lito.host_tool(target, "liproto-codegen", "protoc-gen-liproto")
local proto = lito.external_source(target, "qcm-proto")

local roots = {
  { source = "id.proto", output = "id.cppm" },
  { source = "model.proto", output = "model.cppm" },
  { source = "filter.proto", output = "filter.cppm" },
  { source = "entity.proto", output = "entity.cppm" },
  { source = "message.proto", output = "message.cppm" },
  { source = "google/protobuf/timestamp.proto", output = "google/protobuf/timestamp.cppm" },
}

local inputs = {}
local outputs = {}
local arguments = {
  "-I", "@INPUT_ROOT:1@",
  "--plugin=protoc-gen-liproto=@TOOL:1@",
  "--liproto_out=@OUTPUT_DIR@",
}
for _, item in ipairs(roots) do
  inputs[#inputs + 1] = proto.file(item.source)
  outputs[#outputs + 1] = item.output
  arguments[#arguments + 1] = "@INPUT_ROOT:1@/" .. item.source
end

local generated = lito.run({
  tool = protoc,
  tools = { generator },
  cwd = ".",
  input_roots = { proto },
  args = arguments,
  inputs = inputs,
  outputs = outputs,
})

for index = 1, #roots do
  lito.target_add_generated_source(target, generated.outputs[index])
end
