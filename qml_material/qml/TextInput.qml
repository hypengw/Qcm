import QtQuick

import Qcm.Material as MD

TextInput {
    id: root
    property QtObject typescale: MD.Token.typescale.body_large
    property bool prominent: false

    Binding {
        when: typescale
        root.font.pixelSize: typescale.size
        root.font.weight: root.prominent && typescale.weight_prominent ? typescale.weight_prominent : typescale.weight
        root.font.letterSpacing: typescale.tracking
        restoreMode: Binding.RestoreNone
    }
}