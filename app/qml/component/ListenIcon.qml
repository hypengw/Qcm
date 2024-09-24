import QtQuick
import QtQuick.Layouts

import Qcm.Material as MD

StackLayout {
    id: root
    currentIndex: 0
    property bool isPlaying: false
    property int index: 0
    property int count: 0

    Binding on currentIndex {
        value: 1
        when: root.isPlaying
    }

    MD.Text {
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        typescale: MD.Token.typescale.body_medium
        opacity: 0.6
        text: root.index + 1
    }
    MD.Icon {
        name: MD.Token.icon.equalizer
        size: 24
        color: MD.Token.color.primary
        horizontalAlignment: Qt.AlignHCenter
    }
}
