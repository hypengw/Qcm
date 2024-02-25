import QtQuick
import Qcm.Material as MD

Item {
    id: root
    visible: false

    property QtObject item
    property var ctx: item?.MD.MatProp

    property int elevation
    property color textColor
    property color backgroundColor
    property color supportTextColor
    property color stateLayerColor
}