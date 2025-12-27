import QtQuick
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    default property alias flickData: m_flick.flickableData
    readonly property alias flickable: m_flick

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        leftMargin: 16
        rightMargin: 16
    }
}
