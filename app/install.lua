local lito = require("@lito")

local releases = lito.read_file("assets/Qcm.releases.xml")

lito.install({
  artifacts = {
    {
      target = { kind = "bin", name = "Qcm" },
      destination = "bin/Qcm",
    },
  },
  external_assets = {
    {
      dependency = "qcm-backend",
      set = "QcmBackend",
      destination = "bin",
    },
  },
  files = {
    {
      source = "assets/Qcm.svg",
      destination = "share/icons/hicolor/scalable/apps/io.github.hypengw.Qcm.svg",
    },
  },
  templates = {
    {
      input = "assets/Qcm.desktop.in",
      destination = "share/applications/io.github.hypengw.Qcm.desktop",
      values = {
        APP_ID = "io.github.hypengw.Qcm",
        APP_NAME = "Qcm",
        APP_SUMMARY = "Material You cloud music player",
        PROJECT_NAME = "Qcm",
      },
    },
    {
      input = "assets/Qcm.metainfo.xml.in",
      destination = "share/metainfo/io.github.hypengw.Qcm.metainfo.xml",
      values = {
        APP_ID = "io.github.hypengw.Qcm",
        APP_NAME = "Qcm",
        APP_SUMMARY = "Material You cloud music player",
        APP_RELEASES = releases,
      },
    },
  },
})
