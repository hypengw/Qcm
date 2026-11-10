local lito = require("@lito")
local suffix = lito.target:find("windows") and ".exe" or ""

lito.install({
  artifacts = {
    {
      target = { kind = "bin", name = "qcm-backend-cpp" },
      destination = "bin/QcmBackend" .. suffix,
    },
  },
})
