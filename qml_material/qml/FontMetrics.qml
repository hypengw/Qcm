import QtQuick

import Qcm.Material as MD

FontMetrics {
    id: root
    property MD.t_typescale typescale: MD.Token.typescale.label_medium
    property bool prominent: false

    font.pixelSize: typescale?.size ?? 16
    font.weight: typescale ? (root.prominent && typescale.weight_prominent ? typescale.weight_prominent : typescale.weight) : Font.Normal
    font.letterSpacing: typescale?.tracking ?? 1
}