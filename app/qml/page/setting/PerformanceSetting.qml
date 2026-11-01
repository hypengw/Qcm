pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    title: qsTr("Performance")

    QA.SettingSection {
        width: parent.width
        height: implicitHeight
        title: qsTr("Image memory cache")
        spacing: 2

        QA.SettingRow {
            canInput: false
            text: qsTr("Decode cache: %1 MiB").arg(QA.Global.image_decode_cache_mib)
            supportText: qsTr("Decoded images kept in memory. 0 MiB disables caching.")
            below: MD.Slider {
                objectName: "decodeCacheSlider"
                from: 0
                to: 128
                stepSize: 8
                snapMode: MD.SliderBase.SnapAlways
                value: QA.Global.image_decode_cache_mib
                Accessible.name: qsTr("Decode cache capacity")
                onValueChangeRequested: requestedValue => QA.Global.image_decode_cache_mib = Math.round(requestedValue)
            }
        }

        QA.SettingRow {
            canInput: false
            text: qsTr("GPU cache: %1 MiB").arg(QA.Global.image_gpu_cache_mib)
            supportText: qsTr("Unused image textures kept for reuse. Visible images are not included.")
            below: MD.Slider {
                objectName: "gpuCacheSlider"
                from: 0
                to: 256
                stepSize: 8
                snapMode: MD.SliderBase.SnapAlways
                value: QA.Global.image_gpu_cache_mib
                Accessible.name: qsTr("GPU cache capacity")
                onValueChangeRequested: requestedValue => QA.Global.image_gpu_cache_mib = Math.round(requestedValue)
            }
        }
    }
}
