local lito = require("@lito")
local qt = require("@lito.qt")

local mpris = lito.target({ kind = "lib", name = "qcm-mpris" })
local qt6 = lito.external_dependency(mpris, "qt6")

qt.moc({
  target = mpris,
  qt = qt6,
  files = {
    {
      source = "include/mpris/mpris.h",
      mode = "include",
      output = "mpris/moc_mpris.cpp",
    },
    {
      source = "include/mpris/mediaplayer2.h",
      mode = "include",
      output = "mpris/moc_mediaplayer2.cpp",
    },
    {
      source = "include/mpris/mediaplayer2_adaptor.h",
      mode = "include",
      output = "mpris/moc_mediaplayer2_adaptor.cpp",
    },
  },
})
