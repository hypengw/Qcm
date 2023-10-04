import QtQuick

import Qcm.Material as MD

FontMetrics {
    id: root
    property QtObject typescale: MD.Token.typescale.label_medium
    property bool prominent: false

    font.pixelSize: typescale.size
    font.weight: root.prominent && typescale.weight_prominent ? typescale.weight_prominent : typescale.weight
    font.letterSpacing: typescale.tracking
}