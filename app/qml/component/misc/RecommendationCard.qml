import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

QA.ImagePlayCard {
    id: root
    property var dynamicQueue: null
    property bool radioCover: false
    property QA.item_id coverId
    property string detail
    property string badge

    picWidth: width
    subText: dynamicQueue?.description || detail
    image.source: QA.Util.image_url(radioCover ? dynamicQueue?.currentId : coverId)

    MD.Icon {
        parent: root.image
        anchors.centerIn: parent
        visible: root.radioCover && root.image.source.toString().length === 0
        name: MD.Token.icon.shuffle
        size: 48
    }
    MD.Label {
        parent: root.image
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 8
        visible: root.badge.length > 0
        text: root.badge
        padding: 4
        background: Rectangle {
            color: MD.Token.color.surface
            radius: 4
        }
    }
}
