local target = lito.target({ kind = "lib", name = "qcm-backend" })
local protoc = lito.tool("protoc")
local generator = lito.host_tool(target, "liproto-codegen", "protoc-gen-liproto")
local proto = lito.external_source(target, "qcm-proto")

local roots = {
  "id",
  "model",
  "filter",
  "entity",
  "message",
  "google/protobuf/timestamp",
}

local inputs = {}
local outputs = {}
local arguments = {
  "-I", "@INPUT_ROOT:1@",
  "--plugin=protoc-gen-liproto=@TOOL:1@",
  "--liproto_out=split:@OUTPUT_DIR@",
}
for _, stem in ipairs(roots) do
  local source = stem .. ".proto"
  inputs[#inputs + 1] = proto.file(source)
  outputs[#outputs + 1] = stem .. ".cppm"
  outputs[#outputs + 1] = stem .. ".cpp"
  arguments[#arguments + 1] = "@INPUT_ROOT:1@/" .. source
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

for index = 1, #outputs do
  lito.target_add_generated_source(target, generated.outputs[index])
end
