local lito = require("@lito")
local suffix = lito.target:find("windows") and ".exe" or ""

lito.install({
  artifacts = {
    {
      target = { kind = "bin", name = "qcm" },
      destination = "bin/qcm" .. suffix,
    },
  },
})
